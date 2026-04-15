#pragma once

namespace fs {
	enum class CallbackType : uint16_t {
		Invalid = 0,

		hostQuit = 1,
		hostLostConnection = 2,

		playersCountChanged = 1000,
		newPlayerJoined = 1001,
		playerQuit = 1002,
		syncedPlayersWhenJoined = 1003,

		serverConnectStatus = 2000,
		clientConnectStatus = 2001,
		serverDisconnectStatus = 2002,
		clientDisconnectStatus = 2003,
		
		
		joinedLobby = 3000,
	};

	template<CallbackType T>
	struct CallbackParam;

	/*
	Usage: Host has quited. I am informed
	*/
	template<>
	struct CallbackParam<CallbackType::hostQuit> {
		using type = void;
	};

	/*
	Usage: Host has lost connection. I am informed
	*/
	template<>
	struct CallbackParam<CallbackType::hostLostConnection> {
		using type = void;
	};

	/*
	Usage: Always called for everyone when players have changed. All are informed
	*/
	template<>
	struct CallbackParam<CallbackType::playersCountChanged> {
		using type = void;
	};

	/*
	Usage: a new player has joined the game. All remaining players (excluding player who's just joined) will be informed
	*/
	template<>
	struct CallbackParam<CallbackType::newPlayerJoined> {
		using type = void;
	};

	/*
	Usage: player has left (TODO: problem type unknown for now). All remaining players will be informed
	*/
	template<>
	struct CallbackParam<CallbackType::playerQuit> {
		using type = void;
	};
	
	/*
	Usage: My machine (client) has joined and have been synced with other players. Only i am informed
	*/
	template<>
	struct CallbackParam<CallbackType::syncedPlayersWhenJoined> {
		using type = void;
	};

	/*
	Usage: Called when server connection/disconnection is completed. Returns server status that determines if it was a success or not
	*/
	enum class ServerStatus : uint8_t {
		Success = 0,
		NetworkingNotInitialized,
		SteamNotActive,
		SteamNotRunning,
		SocketInvalid,
		LobbyCreationFailed,
		NewClientConnecting,
		NewClientJoined,
		ClientLeft,
		ClientLostConnection,
	};

	template<>
	struct CallbackParam<CallbackType::serverConnectStatus> {
		using type = ServerStatus;
	};
	
	template<>
	struct CallbackParam<CallbackType::serverDisconnectStatus> {
		using type = ServerStatus;
	};

	/*
	Usage: Called when client connection/disconnection is completed. Returns server status that determines if it was a success or not
	*/
	enum class ClientStatus : uint8_t {
		Success = 0,
		NetworkingNotInitialized,
		SteamNotActive,
		SteamNotRunning,
		InitSuccess,
		InvalidConnection,
		SteamLobbyNotUsed,
		ConnectDataMissing,

		NoHost,
		Connecting,
		Disconnecting,
	};
	
	template<>
	struct CallbackParam<CallbackType::clientConnectStatus> {
		using type = ClientStatus;
	};
	
	template<>
	struct CallbackParam<CallbackType::clientDisconnectStatus> {
		using type = ClientStatus;
	};

	/*
	Usage: I have joined a steam lobby. I am informed
	*/
	template<>
	struct CallbackParam<CallbackType::joinedLobby> {
		using type = void;
	};
}