#pragma once
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/nvp.hpp>

#include <sstream>

#include "steam utilities/Steam.hpp"
#include "ReceiverFlag.hpp"

#include "utilities/IDAllocator.hpp"

#include "utilities/HashDefinitions.hpp"
#include <mutex>
#include <shared_mutex>
/*

DOCUMENTATION
#documentation for this file is completed and provided on the discord under networking/connection

*/

namespace fs {
	class Runtime;
	namespace fs_priv {
		class ConnectionManager;
	}



	struct ConnectionStatus {
		bool valid = false;
		
		//steam net data
		double RTT_ms;
		double qsend_ms;
		float quality;
	};



	class Connection {
	public:
#if USE_STEAM_GNS
		Connection(HSteamNetConnection hSteamNetConnectionID, uint8_t gameID, CSteamID cSteamID, bool isHost = false, bool self = false);

		/*Steam client ID. Use this ID for all steam related requests (obtaining username, profile picture etc.)*/
		CSteamID GetCSteamID() const;
#else
		// Standalone: no CSteamID at all
		Connection(HSteamNetConnection hConn, uint8_t gameID, bool isHost = false, bool self = false);
#endif

		Connection() = default;
		
		
		~Connection() = default;

		/*Steam socket ID. Practically you should use GameID and CSteamID, please check those before obtaining HSteamNetConnectionID.*/
		HSteamNetConnection GetHSteamNetConnectionID() const;
		
		/*Custom GameID abstraction that follows some rules:
		-0 is always an invalid ID
		-1 is always host
		-2,3,4 etc are other client IDs
		*/
		uint8_t GetGameID() const;

		bool IsHost() const;

		/*
		If return type is true, then that means that this specific connection is the one that this machine identifies with.
		It specifies to you what player the server sees you at
		*/
		bool IsSelf() const;

		/*
		Checks if all IDs are valid.
		*/
		bool IsValid() const;

		const ConnectionStatus& GetCurrentConnectionStatus() const;
	private:
		friend class fs_priv::ConnectionManager;
		HSteamNetConnection m_hSteamNetConnectionID = k_HSteamNetConnection_Invalid;
		uint8_t m_gameID = 0; //I.e player 1,2... 0 is invalid /rules for ID assignment are stated above
#if USE_STEAM_GNS
		CSteamID m_cSteamID{};
#endif

		ConnectionStatus m_connectionStatus{};

		fs::ReceiverFlag m_receiverFlag = fs::ReceiverFlag::Invalid; //consult ReceiverFlag file for more information
		
		bool m_isHost = false;
		bool m_isSelf = false;
	private:
		friend class boost::serialization::access;

		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
#if USE_STEAM_GNS
			uint64_t steamId = m_cSteamID.ConvertToUint64();
			ar& steamId;
#endif

			uint16_t receiverRaw = static_cast<uint16_t>(m_receiverFlag);

			ar& m_gameID;
			ar& m_isHost;
			ar& receiverRaw;

			if constexpr (!Archive::is_saving::value) {
#if USE_STEAM_GNS
				m_cSteamID = CSteamID(steamId);
#endif
				m_receiverFlag = static_cast<ReceiverFlag>(receiverRaw);
			}
		}
	};



	namespace fs_priv {
		class ConnectionManager {
		public:
			bool Add(fs::Connection&& connection);

			const fs::Connection& GetByGameID(uint8_t gameID) const;
			const fs::Connection& GetByHSteamNetConnectionID(HSteamNetConnection hSteamNetConnectionID) const;

			bool RemoveByGameID(uint8_t gameID);
			bool RemoveByHSteamNetConnectionID(HSteamNetConnection hSteamNetConnectionID);

#if USE_STEAM_GNS
			const fs::Connection& GetByCSteamID(CSteamID cSteamID) const;
			bool RemoveByCSteamID(CSteamID cSteamID);
#endif

			const fs::Connection& GetHost() const;
			const fs::Connection& GetSelf() const;

			std::vector<fs::Connection> GetAllCopies();

			/*
			It is safe to call this function from multiple threads
			*/
			std::vector<HSteamNetConnection> GetAllHSteamConnectionIDs() const;
			
			/*
			It is safe to call this function from multiple threads
			*/
			uint8_t GetGameIDFromHSteamNetConnectionID(HSteamNetConnection id) const;
			/*
			It is safe to call this function from multiple threads
			*/
			HSteamNetConnection GetHSteamNetConnectionIDFromGameID(uint8_t id) const;

			size_t GetCount() const;

			void Destroy();

			const std::unordered_map<uint8_t, fs::Connection>& GetAll() const;

			void MakeSelf(fs::Connection* whoToMakeSelf);
			void MakeHost(fs::Connection* whoToMakeHost);

			/*
			NOTE: you have to do something with this id if you retrieve it, otherwise you're just occupying a slot
			*/
			uint16_t RetrieveNextUniqueID();

			void UpdateHostConnectionStatus();
		private:
			bool Remove(uint8_t gameID);
			friend class fs::Connection;
			friend class NetworkingManager;

			fs::Connection* m_host = nullptr;
			fs::Connection* m_self = nullptr;

			
			std::unordered_map<uint8_t, fs::Connection> m_connectionsGameID; //the actual storage
			std::unordered_map<HSteamNetConnection, fs::Connection*> m_connectionsHSteamNetConnectionID;
#if USE_STEAM_GNS
			std::unordered_map<CSteamID, fs::Connection*> m_connectionsCSteamID;
#endif
			fs::IDAllocator m_IDAllocator{};

			mutable std::shared_mutex m_connectionMutex;
		};
	}



	inline fs_priv::ConnectionManager connections;
}