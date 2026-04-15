#pragma once
#if USE_STEAM_GNS
#include "NetworkingManager.hpp"

namespace fs {
	namespace fs_priv {
		class NetworkingSteam {
			bool Active() const;
		private:
			friend class NetworkingManager;

			bool Init();
			bool Shutdown();

			void RunCallbacks();

			bool StartListeningRemote();
			bool StartListeningLocal(SteamNetworkingIPAddr hostingAddress);
			bool StopListening();

			bool ConnectToServerLocal(SteamNetworkingIPAddr serverAddress);
			bool DisconnectFromServer();

			const CSteamID& GetLobbyID();
			
			bool IsServer() const;

			//callbacks
			void OnLobbyCreated(LobbyCreated_t* pCallback);
			void OnJoinRequested(GameLobbyJoinRequested_t* pCallback);
			void OnLobbyEnter(LobbyEnter_t* pCallback);

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
			bool m_connectingToServer = false;

			HSteamNetConnection m_listeningSocket{};
			CSteamID m_currentLobbyID;

			//Steam callbacks
			CCallbackManual<NetworkingManager, SteamNetConnectionStatusChangedCallback_t>	m_connectionHandlerCallback;
			CCallbackManual<NetworkingSteam, LobbyCreated_t>								m_lobbyCreatedCallback;
			CCallbackManual<NetworkingSteam, GameLobbyJoinRequested_t>					m_joinRequestCallback;
			CCallbackManual<NetworkingSteam, LobbyEnter_t>								m_lobbyEnterCallback;
		};

		inline NetworkingSteam networkingImpl;
	}
}
#endif