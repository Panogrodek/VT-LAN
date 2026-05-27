#pragma once
#include "Callback.hpp"
#include "RPC.hpp"
#include "Connection.hpp"
#include "ReceivingThread.hpp"
#include "ReceiverFlag.hpp"
#include "SerializationHelpers.hpp"

#include "steam utilities/Steam.hpp"

#if USE_STEAM_GNS
#include <steam/steam_api.h>
#endif

/*

DOCUMENTATION
#documentation for this file is completed and provided on the discord under networking/networking

*/

namespace fs {
	enum class NetworkingHostingMode : uint8_t {
		Invalid = 0,
		Local,
		Remote
	};


	class Runtime;
	namespace fs_priv {
		class NetworkingSteam;
		class NetworkingGNS;

		class NetworkingManager {
		public:
			bool StartListening(NetworkingHostingMode mode, SteamNetworkingIPAddr localHostingAddress = SteamNetworkingIPAddr{});
			//return true if the disconnection was graceful
			bool StopListening();

			bool ConnectToServer(SteamNetworkingIPAddr serverAddress);
			//return true if the disconnection was graceful
			bool DisconnectFromServer();

			bool Server() const;
			bool Client() const;
			bool ActiveSession() const;

			const CSteamID& GetLobbyId() const;

			uint8_t GetMaxClientCount();
		private:
			friend class Connection;
			friend class Sender;
			friend class Receiver;
			friend class ConnectionManager;
			friend class NetworkingSteam;
			friend class NetworkingGNS;
			friend class fs::Runtime;


			void HandleConnections(SteamNetConnectionStatusChangedCallback_t* pCallback);


			//all of these need to be in runtime, thus private
			void Init();
			void Shutdown();
			void ReceiveMessagesPass();
			void SendMessagesPass();

			void RunCallbacks();

			//HandleConnectionFunctions

			bool m_initialized = false;
			uint8_t m_maxConnections = 4; //TODO: move it somewhere else
		};
	}

	inline fs_priv::NetworkingManager networking;
}