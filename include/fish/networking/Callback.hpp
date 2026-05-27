#pragma once
#include "CallbackParamList.hpp"
#include <unordered_map>
#include <functional>

/*

DOCUMENTATION
#documentation for this file is completed and provided on the discord under networking/callback

*/

//TODO: it might not be memory safe

namespace fs {
	namespace fs_priv {
		template<typename Args>
		class CallbackBase {
		public:
			virtual ~CallbackBase() = default;
			virtual void Call(Args&& args) const = 0;
		};

		// Specialization for void
		template<>
		class CallbackBase<void> {
		public:
			virtual ~CallbackBase() = default;
			virtual void Call() const = 0;
		};
	}
	


	template<CallbackType T, typename Param = typename CallbackParam<T>::type>
	class Callback : public fs_priv::CallbackBase<Param> {
	public:
		using FunctionType = std::function<void(Param)>;

		Callback() = default;
		Callback(FunctionType func) : m_func(std::move(func)) {};
		~Callback() = default;

		void Register(FunctionType func) {
		#ifdef DEBUG
			if (m_func != nullptr) {
				WLOG("Overriding callback function handling type id: {}", static_cast<uint16_t>(T));
			}
		#endif

			m_func = std::move(func);
		}

		void Call(Param&& param) const final {
			if (m_func)
				m_func(param);
		}

	private:
		FunctionType m_func = nullptr;
	};
	


	template<CallbackType T>
	class Callback<T, void> : public fs_priv::CallbackBase<void> {
	public:
		using FunctionType = std::function<void()>;

		Callback() = default;
		Callback(FunctionType func) : m_func(std::move(func)) {}
		~Callback() = default;

		void Register(FunctionType func) {
		#ifdef DEBUG
			if (m_func != nullptr) {
				WLOG("Overriding callback function handling type id: {}", static_cast<uint16_t>(T));
			}
		#endif
			m_func = std::move(func);
		}

		void Call() const final {
			if (m_func)
				m_func();
		}

	private:
		FunctionType m_func = nullptr;
	};



	namespace fs_priv {
		class CallbackStorageElementBase {
		public:
			virtual ~CallbackStorageElementBase() = default;
		};

		template<typename Args>
		class CallbackStorageElement : public CallbackStorageElementBase {
		public:
			void Add(CallbackBase<Args>* callback) {
				m_callbacks.emplace_back(callback);

				if (callbacks.GetCallbackGroup() != nullptr)
					callbacks.GetCallbackGroup()->Track(*this, callback);
			}

			void Remove(CallbackBase<Args>* callback) {
				m_callbacks.erase(std::remove(m_callbacks.begin(), m_callbacks.end(), callback), m_callbacks.end());
			}

			void CallAll(Args&& arg) {
				for (auto* callback : m_callbacks)
					callback->Call(std::forward<Args>(arg));
			}
		private:
			std::vector<CallbackBase<Args>*> m_callbacks;
		};

		template<>
		class CallbackStorageElement<void> : public CallbackStorageElementBase {
		public:
			void Add(CallbackBase<void>* callback);
			void Remove(CallbackBase<void>* callback);
			void CallAll();

		private:
			std::vector<CallbackBase<void>*> m_callbacks;
		};
	}

	/*
	Usage:

	Use CallbackGroup when you're setting up custom user defined callbacks that are only needed temporarily,
	such as during specific states of your application (e.g., lobby state player synchronization).

	CallbackGroup groups and stores all callbacks created during its lifetime.

	Once you're done with that application state (e.g., when leaving the lobby state), you can
	destroy the CallbackGroup to automatically unregister and clean up those callbacks.

	This helps to free memory and ensures that callbacks defined in this group will be freed and
	won't be called
	*/
	class CallbackGroup {
	public:
		CallbackGroup() { Bind(); };
		~CallbackGroup() { UnBind(); };

		void UnBind();
		void Bind();
		void Free();

		template<typename Args>
		void Track(fs_priv::CallbackStorageElement<Args>& storage, fs_priv::CallbackBase<Args>* callback) {
			m_callbacks.push_back({
				[&storage, callback]() {
					storage.Remove(callback);
					delete callback;
				}
				});
		}

	private:
		struct CallbackEntry {
			std::function<void()> removeCallback;
		};

		std::vector<CallbackEntry> m_callbacks;
	};



	namespace fs_priv {
		class CallbackStorage {
		public:

			void Destroy() {
				for (auto& [_, ptr] : m_storage)
					delete ptr;
				m_storage.clear();
			}

			template<fs::CallbackType T>
			CallbackStorageElement<typename fs::CallbackParam<T>::type>& Get() {
				using Arg = typename fs::CallbackParam<T>::type;

				auto it = m_storage.find(T);
				if (it == m_storage.end()) {
					auto* element = new CallbackStorageElement<Arg>();
					m_storage[T] = element;
					return *element;
				}

				return *static_cast<CallbackStorageElement<Arg>*>(m_storage[T]);
			}

			fs::CallbackGroup*& GetCallbackGroup() { return m_activeCallbackGroup; };
		private:
			fs::CallbackGroup* m_activeCallbackGroup = nullptr;
			std::unordered_map<fs::CallbackType, CallbackStorageElementBase*> m_storage;
		};
	}



	inline fs_priv::CallbackStorage callbacks;
}