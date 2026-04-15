#pragma once
#include "PhysicsWorld.hpp"
#include "PhysicsNetworkingData.hpp"
#include "networking/ReceiverFlag.hpp"
#include <set>

namespace fs {
	class PhysicsNetworkingWorld;

	namespace fs_priv {
		class PhysicsNetworkingServer {
		public:
			uint32_t GetCurrentTick() const;

			fs::PhysicsWorld& GetWorld();

			void GrantPlayerAccess(uint32_t physicsBodyID, fs::ReceiverFlag playerID);
			void RevokePlayerAccess(fs::ReceiverFlag playerID);
		private:
			bool Init(b2WorldDef worldDef, uint8_t networkingWorldID);
			void Shutdown();
			void StartCreateRemove();
			void StartNonStaticMovement();

			void Step();

			void NetworkReceivePass();
			void NetworkSendPass();


			void PhysicsWorldAddRemoveSend();

			void PhysicsWorldCreateSend(const std::set<uint32_t>& createdIds);
			void PhysicsWorldRemoveSend();
		private:
			friend class fs::PhysicsNetworkingWorld;
			uint32_t m_serverTick = 0; //MAX for invalid tick
			int32_t m_serverSyncWait = 0;
			int32_t m_createRemoveWait = 0;

			uint8_t m_networkingWorldID = INVALID_PHYSICS_ID;

			bool m_initialized = false;
			bool m_startCreateRemove = false;
			bool m_startNonStaticMovement = false;

			fs::PhysicsWorld m_serverWorld; //authoritative, server world

			//value stored - bodyID of physics body
			std::unordered_map<fs::ReceiverFlag, uint32_t> m_playerBodyRelations;

			//key - tick
			std::map<uint32_t, std::unordered_map<fs::ReceiverFlag, PhysicsClientInputData>> m_playersInput;
		};
	}
}