#pragma once
#include <any>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include "Sender.hpp"

#include <unordered_map>
#include <vector>
#include <list>
#include <unordered_set>
#include <tuple>
#include <deque>

#include <networking/ReceiverFlag.hpp>
#include <utilities/Macros.hpp>

/*

DOCUMENTATION
#documentation for this file is completed and provided on the discord under networking/RPC

*/

//---------------------------------------------------------------------
// Utility: function_traits to deduce callable signature
//---------------------------------------------------------------------
template<typename T>
struct function_traits;

// Specialization for function pointers
template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
};

// Specialization for std::function
template<typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
};

// Specialization for member function pointers (const)
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...) const> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
};

// Specialization for lambdas and functors
template<typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};

//---------------------------------------------------------------------
// Serialization utilities for tuple
//---------------------------------------------------------------------
template <class Archive, std::size_t Index = 0, typename... Types>
typename std::enable_if<Index == sizeof...(Types), void>::type
serialize_tuple(Archive&, std::tuple<Types...>&) {}

template <class Archive, std::size_t Index = 0, typename... Types>
typename std::enable_if < Index < sizeof...(Types), void>::type
    serialize_tuple(Archive& ar, std::tuple<Types...>& t) {
    ar& std::get<Index>(t);
    serialize_tuple<Archive, Index + 1>(ar, t);
}

namespace boost {
    namespace serialization {
        template <class Archive, typename... Types>
        void serialize(Archive& ar, std::tuple<Types...>& t, const unsigned int) {
            serialize_tuple(ar, t);
        }
    }
}

//---------------------------------------------------------------------
// RPC and helper functions
//---------------------------------------------------------------------
namespace fs {
    enum class Authority : uint8_t { //TODO: This REALLY shouldn't be here
        Invalid = 0, HostOnly = 1, All = 2,
    };

    // Forward declaration corner
    class Runtime;
	namespace fs_priv {
		class RPCMap;
	}


    template <typename T>
    inline std::string serializeData(const T& data) {
        std::ostringstream oss;
        boost::archive::binary_oarchive oa(oss);
        oa << data;
        return oss.str();
    }

    template <typename T>
    inline T deserializeData(const std::string& serializedData) {
        T data;
        std::istringstream iss(serializedData);
        boost::archive::binary_iarchive ia(iss);
        ia >> data;
        return data;
    }

    class RPC {
    public:
        template <typename ...Args>
        void Call(Args&&... args) const;

        void CallSerialized(const std::string& serializedArgs) const;

        const Authority& GetAuthority() const;

        void BindValidation(std::function<bool(fs::ReceiverFlag)> validation);
        bool Validate(fs::ReceiverFlag requester) const;

        template <typename ...Args>
        std::string SerializeData(Args... args) const;

        uint16_t GetId() const;
    private:
        template <typename F>
        void Init(F&& func, uint16_t id, const Authority& auth);

		friend class fs_priv::RPCMap;
        Authority m_auth = Authority::Invalid;
        uint16_t m_id = 0; // unknown
        std::function<void(const std::string&)> m_func;
        std::function<bool(fs::ReceiverFlag)> m_validation = nullptr;
    };

    template<typename F>
    inline void RPC::Init(F&& func, uint16_t id, const Authority& auth)
    {
        // Deduce the argument tuple type from the callable.
        using traits = function_traits<std::decay_t<F>>;
        using args_tuple_t = typename traits::args_tuple;
        m_id = id;
        m_auth = auth;
        m_func = [func = std::forward<F>(func)](const std::string& serializedArgs) {
		#ifdef DEBUG
            try {
                auto args = deserializeData<args_tuple_t>(serializedArgs);
                std::apply(func, args);
            }
            catch (const std::exception& e) {
                ALOG("RPC call failed during deserialization: {}",e.what());
            }
            
		#else
			auto args = deserializeData<args_tuple_t>(serializedArgs);
			std::apply(func, args);
		#endif 
		};
    }

    template<typename ...Args>
    inline void RPC::Call(Args&& ...args) const
    {
		if (m_id == 0 || m_id == UINT16_MAX)
			return;
		if (m_auth == Authority::Invalid) {
			ELOG("Cannot call RPC, Authority is invalid!");
			return;
		}
        auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
        std::string serializedArgs = serializeData(argsTuple);
        m_func(serializedArgs);
    }

    template<typename ...Args>
    inline std::string RPC::SerializeData(Args ...args) const
    {
        auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
        return serializeData(argsTuple);
    }

    inline void RPC::CallSerialized(const std::string& serializedArgs) const
    {
		if (m_id == 0 || m_id == UINT16_MAX)
			return;
		if (m_auth == Authority::Invalid) {
			ELOG("Cannot call serialized RPC, Authority is invalid!");
			return;
		}
        m_func(serializedArgs);
    }

    inline const Authority& RPC::GetAuthority() const
    {
        return m_auth;
    }

    inline void RPC::BindValidation(std::function<bool(fs::ReceiverFlag)> validation)
    {
        m_validation = validation;
    }

    inline bool RPC::Validate(fs::ReceiverFlag requester) const
    {
        if (m_validation == nullptr)
            return true;
        return m_validation(requester);
    }

    inline uint16_t RPC::GetId() const
    {
        return m_id;
    }

	//---------------------------------------------------------------------
	// RPCMap managing a collection of RPCs
	//---------------------------------------------------------------------
	class RPCRegistrar;
	namespace fs_priv {
		class RPCMap {
		public:

			template<typename ...Args>
			void Call(const std::string& name, Args&&... args);

			void Call(uint16_t id, const std::string& serializedArgs);

			uint16_t GetRPCId(const std::string& name);

			const fs::RPC* GetRPC(const std::string& name) const;
			const fs::RPC* GetRPC(uint16_t id) const;
			
			void Lock();
		private:
			friend class RPCRegistrar;
			// Now accepts any callable F.
			template<typename F>
			void Add(const std::string& name, const fs::Authority& auth, F&& func);

			friend class fs::Runtime;
			//void Shutdown(); //this is not safe to be fair, it is better to leak it to memory

			bool m_closed = false;
			std::unordered_map<std::string, fs::RPC*> m_rpcs;
			std::unordered_map<uint16_t, fs::RPC*> m_rpcsIDs;
		};

		template<typename F>
		inline void RPCMap::Add(const std::string& name, const fs::Authority& auth, F&& func)
		{
			if (m_closed) {
				ALOG("Attempting to add new RPC after closing!!!");
				return;
			}

			if(m_rpcs.find(name) != m_rpcs.end()){
				ALOG("Attempting to overwrite existing RPC!!!");
				return;
			}

			fs::RPC* temp = new fs::RPC();
			uint16_t id = m_rpcs.size() + 1; //so that 1 is unknown;
			temp->Init(std::forward<F>(func), id, auth);

			auto [it, inserted] = m_rpcs.insert({ name, temp });

			if (!inserted) {
				ALOG("Critical error whilst inserting RPC");
				delete temp;
				return;
			}
			m_rpcsIDs[id] = m_rpcs[name];
		}

		template<typename ...Args>
		inline void RPCMap::Call(const std::string& name, Args&& ...args)
		{
			if (m_rpcs.find(name) == m_rpcs.end()) {
				ALOG("Attempting to call non existing RPC!!!");
				return;
			}
			if (m_rpcs[name] == nullptr) {
				WLOG("RPC named: {} is corrupted", name);
				return; //TODO: figure out what happens in this state (though this state should not happen)
			}
			m_rpcs[name]->Call(std::forward<Args>(args)...);
		}

		inline void RPCMap::Call(uint16_t id, const std::string& serializedArgs)
		{
			if (m_rpcsIDs.find(id) == m_rpcsIDs.end()) {
				ALOG("Attempting to call non existing RPC!!!");
				return;
			}

			if (m_rpcsIDs[id] == nullptr) {
				WLOG("RPC ID: {} is corrupted", id);
				return; //TODO: figure out what happens in this state (though this state should not happen)
			}

			m_rpcsIDs[id]->CallSerialized(serializedArgs);
		}

		//inline void RPCMap::Shutdown()
		//{
		//	for (auto& [name, rpc] : m_rpcs) {
		//		delete rpc;
		//	}
		//	m_rpcs.clear();
		//}

		inline uint16_t RPCMap::GetRPCId(const std::string& name)
		{
			if (m_rpcs.find(name) == m_rpcs.end()) {
				WLOG("Attempting to get ID from non existing RPC named: {}", name);
				return 0;
			}

			return m_rpcs[name]->GetId();
		}

		inline const fs::RPC* RPCMap::GetRPC(const std::string& name) const
		{
			if (m_rpcs.find(name) == m_rpcs.end()) {
				WLOG("Attempting to get non existing RPC named: {}", name);
				return nullptr;
			}

			return m_rpcs.at(name);
		}
		inline const fs::RPC* RPCMap::GetRPC(uint16_t id) const
		{
			if (m_rpcsIDs.find(id) == m_rpcsIDs.end()) {
				WLOG("Attempting to get non existing RPC ID: {}", id);
				return nullptr;
			}

			return m_rpcsIDs.at(id);
		}
	}

	inline fs_priv::RPCMap& getRPCMap() {
		static fs_priv::RPCMap rpcMapObject;
		return rpcMapObject;
	}

	//---------------------------------------------------------------------
	// RPC map accessor and registrar helpers
	//---------------------------------------------------------------------

	struct RPCRegistrar {
		// General callable registration.
		template<typename F>
		RPCRegistrar(const char* name, const Authority& auth, F&& func) {
			getRPCMap().Add(name, auth, std::forward<F>(func));
		}
		// Overload for singleton member functions.
		template<typename T, typename ...Args>
		RPCRegistrar(const char* name, const Authority& auth, T& instance, void(T::* func)(Args...)) {
			getRPCMap().Add(name, auth, [&instance, func](Args... args) {
				(instance.*func)(args...);
				});
		}
	};

	//maybe add inline, and register functions in hpp if cpp gets undefined behaviour

#define REGISTER_FUNC_RPC(name, authority, func) \
        static fs::RPCRegistrar rpc_func___COUNTER__(name, authority, func);

#define REGISTER_SINGLETON_RPC(name, authority, singleton, func) \
        static fs::RPCRegistrar rpc_singleton___COUNTER__(name,authority, singleton, func);

}



