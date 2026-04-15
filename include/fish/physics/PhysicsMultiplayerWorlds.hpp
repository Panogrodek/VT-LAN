#pragma once
#include "PhysicsNetworkingWorld.hpp"
#include "PhysicsNetworkingData.hpp"

#include "networking/ReceiverFlag.hpp"

namespace fs {
	class Runtime;

	namespace fs_priv {
		constexpr uint16_t MIN_CLIENT_DESIRED_LEAD = 3;  //in physics ticks ~ 45 ms RTT/2 (for 60 tick sim)
		constexpr uint16_t MAX_CLIENT_DESIRED_LEAD = 10; //in physics ticks ~ 150ms RTT/2 (for 60 tick sim)
		/*
		Defines the minimum/maximum lead that the client has over server.
		The lead stands for the amount of ticks client simulates "in the future" over the
		server in real time. If clients RTT is over 2*MAX_CLIENT_DESIRED_LEAD the simulation will suffer greatly
		*/


		constexpr uint16_t SERVER_WAIT_TIME = 5; //in physics ticks ~ 80ms RTT (for 60 tick sim) (TODO: this is to little as this is RTT actually)
		/*
		The base tick wait time the server will wait after receiving StartNonStaticMovement command.
		This is used to slow the server down in time, so that the clients can simulate the future on their machines
		*/


		constexpr uint32_t BODY_HOLD_TIME = 255; //in networking ticks ~ 4,25 s (for 60Hz net send)
		/*
		The tick wait time that clients hold bodies that are suppoused to be added into worlds that don't exist.
		Example:
		Server sends Create signal for world ID: 1
		If client doesn't create this world for BODY_HOLD_TIME ticks from receiving this signal, its dropped
		*/


		constexpr uint16_t MAX_CLIENT_LOCAL_LEAD = 30; //in physics ticks ~ 500ms (for 60 tick sim)
		/*
		Defines the maximum amount of lead that the client can have.
		The difference between MAX_CLIENT_LOCAL_LEAD and MAX_CLIENT_DESIRED_LEAD is simple:
		MAX_CLIENT_DESIRED_LEAD -> this is calculated based on ping, lattency, jitter and other telemetry. 
		It defines how the clients simulation should behave compared to server based on his network

		MAX_CLIENT_LOCAL_LEAD -> this is the maximum amount that we can simulate our local physics world without having any
		server packets received. So, if the client hasn't received any server packets for MAX_CLIENT_LOCAL_LEAD ticks, then 
		the simulation stops.

		MAX_CLIENT_LOCAL_LEAD should always be greater than MAX_CLIENT_DESIRED_LEAD
		*/


		class PhysicsMultiplayerWorldManager {
		public:
			//returns ID of the world, returns INVALID_PHYSICS_ID if not added
			uint8_t Add(b2WorldDef worldDef, const std::string& worldName = "");

			fs::PhysicsNetworkingWorld* GetByName(const std::string& worldName);
			fs::PhysicsNetworkingWorld* GetByID(uint8_t worldID);


			bool RemoveByName(const std::string& worldName);
			bool RemoveByID(uint8_t worldID);

			uint16_t GetClientDesiredLead() const;
		private:
			friend class fs::Runtime;

			bool Init();
			void Shutdown();

			bool Erase(uint8_t worldID);
			//bool Insert();

			void Step();

			void NetworkPushReceivedData();
			void NetworkReceivePass();
			void NetworkSendPass();

			void UpdateClientLead();

			void NetworkReceivePushCreateRemove();
		private:
			bool m_initialized = false;

			fs::IDAllocator m_worldAllocator;

			uint16_t m_clientDesiredLead = 0; //0 for invalid (TODO: unless MIN_CLIENT_DESIRED_LEAD set to 0 instead, maybe dont do that)

			std::unordered_map<uint8_t, fs::PhysicsNetworkingWorld> m_worlds;
			std::unordered_map<std::string, fs::PhysicsNetworkingWorld*> m_worldsByName;

			//world, ttl, packet on hold
			std::unordered_map<uint8_t, std::vector<std::pair<uint32_t, PhysicsCreatePacket>>> m_createHolding;
			std::unordered_map<uint8_t, std::vector<std::pair<uint32_t, PhysicsRemovePacket>>> m_removeHolding;
		};
	}

	inline fs_priv::PhysicsMultiplayerWorldManager multiplayerWorldManager;
}