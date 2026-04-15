#pragma once
#include "PhysicsNetworkingServer.hpp"
#include "PhysicsNetworkingClient.hpp"
#include "PhysicsNetworkingScript.hpp"
#include "utilities/IDAllocator.hpp"

#include <unordered_map>

namespace fs {
	class Runtime;
	namespace fs_priv {
		class PhysicsMultiplayerWorldManager;

		inline constexpr const char* createChannelName = "private_physicsCreateChannel";
		inline constexpr const char* removeChannelName = "private_physicsRemoveChannel";
		inline constexpr const char* syncChannelName   = "private_physicsSyncChannel";
		inline constexpr const char* inputChannelName  = "private_physicsInputChannel";
	}

	class PhysicsNetworkingWorld {
	public:
		fs_priv::PhysicsNetworkingClient& AsClient();
		fs_priv::PhysicsNetworkingServer& AsServer();

		/*
		USAGE: unlocks the ability for the physics world to send/erase bodies. Usefull if we want to create the map without actually letting players move
		in it.

		This takes lag into account and will delay send/remove signal calls by SERVER_WAIT_TIME variable
		(The bodies will be kept on the client for BODY_HOLD_TIME amount of ticks. After that, the client will discard them (which will undoubtedly to desync))
		*/
		void StartCreateRemove();

		/*
		USAGE: unlocks the ability for the physics world to be processed. This function should be called after all the initial bodies are created

		This takes lag into account and will delay server start time by SERVER_WAIT_TIME variable
		*/
		void StartNonStaticMovement();

		void AddScript(PhysicsNetworkingScript&& scirpt);
		const std::unordered_map<uint16_t, PhysicsNetworkingScript>& GetScripts() const;
	private:
		friend class Runtime;
		friend class fs_priv::PhysicsMultiplayerWorldManager;
		
		bool Init(b2WorldDef worldDef);
		void Shutdown();

		void Step();

		void NetworkReceivePass();
		void NetworkSendPass();
	private:

		std::string m_physicsNetworkingWorldName{};
		uint8_t m_physicsNetworkingWorldID = INVALID_PHYSICS_ID;

		bool m_initialized = false;
		bool m_createRemoveStarted = false;
		bool m_syncStarted = false;

		IDAllocator m_scriptIDs{};
		std::unordered_map<uint16_t, PhysicsNetworkingScript> m_scripts;

		fs_priv::PhysicsNetworkingServer m_server;
		fs_priv::PhysicsNetworkingClient m_client;
	};
}