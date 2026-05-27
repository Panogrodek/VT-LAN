#pragma once
#if !USE_STEAM_GNS
#include "NetworkingManager.hpp"

namespace fs {
	namespace fs_priv {
		class NetworkingGNS {
			bool Active() const;
		private:
			friend class NetworkingManager;

			bool Init();
			bool Shutdown();

			void RunCallbacks();

			bool StartListeningLocal(SteamNetworkingIPAddr hostingAddress);
			bool StopListening();

			bool ConnectToServerLocal(SteamNetworkingIPAddr serverAddress);
			bool DisconnectFromServer();

			const CSteamID& GetLobbyID();

			bool IsServer() const;
			bool IsClient() const;

			static void HandleConnectionHelper(SteamNetConnectionStatusChangedCallback_t* callback);

			//all the steam callbacks
			void ConnectionStateConnecting(SteamNetConnectionStatusChangedCallback_t* pCallback);
			void ConnectionStateFindingRoute(SteamNetConnectionStatusChangedCallback_t* pCallback);
			void ConnectionStateConnected(SteamNetConnectionStatusChangedCallback_t* pCallback);
			void ConnectionStateClosedByPeer(SteamNetConnectionStatusChangedCallback_t* pCallback);
			void ConnectionStateProblemDetectedLocally(SteamNetConnectionStatusChangedCallback_t* pCallback);
			void ConnectionStateFinWait(SteamNetConnectionStatusChangedCallback_t* pCallback);
			void ConnectionStateLinger(SteamNetConnectionStatusChangedCallback_t* pCallback);
			void ConnectionStateDead(SteamNetConnectionStatusChangedCallback_t* pCallback);
		private:
			bool m_listening = false;
			bool m_clientStatus = false;

			HSteamNetConnection m_listeningSocket{};
			CSteamID m_currentLobbyID{}; //<- this value is not used by GNS, only SteamGNS
		};

		inline NetworkingGNS networkingImpl;
	}
}
#endif