#pragma once
#include <unordered_set>
#include <utility>
#include <set>
//#include <box2d/box2d.h>

#include "utilities/IDAllocator.hpp"
#include "PhysicsBody.hpp"

namespace fs {
	class Runtime;
	class PhysicsNetworkingWorld;

	namespace fs_priv {
		class PhysicsSingleplayerWorldManager;

		class PhysicsNetworkingClient;
		class PhysicsNetworkingServer;

		#define DEFAULT_SUB_STEP_COUNT 5

		#define BOX2D_MAX_WORLDS 128
	}



	class PhysicsWorld {
	public:
		//returns body custom ID, INVALID_PHYSICS_ID if body not added
		uint32_t Add(fs::PhysicsBody&& body);

		//returns true if body exists, false otherwise
		const fs::PhysicsBody* GetByName(const std::string& name);
		const fs::PhysicsBody* GetByID(uint32_t customId);

		const std::unordered_map<uint32_t, fs::PhysicsBody>& GetAll() const;

		void RemoveByName(const std::string& name);
		void RemoveByCustomID(uint32_t customID);
	private:
		friend class Runtime;
		friend class fs_priv::PhysicsSingleplayerWorldManager;

		friend class fs_priv::PhysicsNetworkingClient;
		friend class fs_priv::PhysicsNetworkingServer;

		void Init(b2WorldDef b2WorldDefinition);
		void Shutdown();

		void Create();


		bool Erase(uint32_t id);
		uint32_t Insert(fs::PhysicsBody&& body); 		//returns body custom ID, INVALID_PHYSICS_ID if body not added


		//networking receive		
		//void NetworkSyncReceive(fs_priv::PhysicsSyncPacket body);


		////networking send
		//void NetworkSend();

		//void NetworkCreateSend(const std::set<uint32_t>& createdIds);
		//void NetworkRemoveSend();
		//void NetworkSyncSend();


		//world
		void Step();
	private:
		std::string m_physicsWorldName = "";
		uint8_t m_physicsWorldID = INVALID_PHYSICS_ID; //own ID

		
		//box2d defines
		b2WorldDef m_worldDef;
		b2WorldId  m_worldID;


		fs::IDAllocator m_bodyAllocator;


		bool m_initialized = false;


		//stored data
		std::unordered_map<uint32_t, fs::PhysicsBody> m_bodies; //<-this holds all bodies (including nonStaticBodies)


		std::unordered_map<std::string, fs::PhysicsBody*> m_stringDictionary;


		//networking helper sets that are only used when the world is synced through network
		std::unordered_set<fs::PhysicsBody*> m_nonStaticBodies;
		std::set<fs::PhysicsBody*> m_bodiesToCreate;
		std::set<uint32_t> m_bodiesToRemove;
	};
}