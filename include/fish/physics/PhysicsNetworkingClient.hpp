#pragma once

#include "PhysicsWorld.hpp"
#include "PhysicsNetworkingData.hpp"

namespace fs {
	class PhysicsNetworkingWorld;
	class PhysicsNetworkingScript;

	namespace fs_priv {
		class PhysicsMultiplayerWorldManager;

		class PhysicsNetworkingClient {
		public:
			PhysicsWorld& GetWorld();

			uint32_t GetCurrentPredictionTick() const;
			uint32_t GetLastKnownServerTick() const;

			//this has to be received by the server, it won't let you modify any body
			void SetPlayerBodyID(uint32_t bodyID);

			//this is set for the current physics tick AND THEN RESET, if you fail to set this, it will be none
			void SetPlayerInputData(PhysicsClientInputMoveDesire move, int64_t userData = 0);
		private:
			bool Init(b2WorldDef worldDef, uint8_t networkingWorldID);
			void Shutdown();
			void StartCreateRemove();
			void StartNonStaticMovement();
			
			void Step();

			int CalculateCurrentSteps();

			void NetworkReceivePass();
			void NetworkSendPass();

			void AuthoritativePlayerUpdate();
		private:
			friend class fs::PhysicsNetworkingWorld;
			friend class fs_priv::PhysicsMultiplayerWorldManager;
			friend class fs::PhysicsNetworkingScript;
			//lead

			uint16_t m_clientCurrentLead = 0;
			//uint16_t m_clientDesiredLead = 0; -> in WorldManager as it is the same for all worlds
			
			//ticks
			uint32_t m_clientPredictionTick = 0; //0 for invalid
			uint32_t m_lastKnownServerTick = 0; //0 for invalid
			bool m_canDecreaseLead = true;

			bool m_initialized = false;
			bool m_startCreateRemove = false;
			bool m_startNonStaticMovement = false;

			uint8_t m_networkingWorldID = INVALID_PHYSICS_ID;

			fs::PhysicsWorld m_clientPredictionWorld; //actual world that we use for client prediction
			fs::PhysicsWorld m_clientReconciliationWorld; //buffer world, used for reconciliation calculation

			std::vector<PhysicsCreatePacket> m_createDataReceived;
			std::vector<PhysicsRemovePacket> m_removeDataReceived;
			//where key - bodyID, value - its sync packet
			std::unordered_map<uint32_t,PhysicsSyncPacket> m_syncDataReceived;

			uint32_t m_playerID = INVALID_PHYSICS_ID; //the body of the player
			PhysicsClientInputData m_inputData{}; //reset every step
			
			std::unordered_map<uint32_t, PhysicsClientInputData> m_userPlayerInputHistoryLocal;
			std::vector<PhysicsClientInputPacket> m_userInputToSend;
		};
	}
}