#pragma once
#include "PhysicsWorld.hpp"

#include <unordered_set>
#include <set>
#include <utility>

namespace fs {
	namespace fs_priv {
		class PhysicsSingleplayerWorldManager {
		public:
			//returns ID of the world, returns INVALID_PHYSICS_ID if not added
			uint8_t Add(fs::PhysicsWorld&& world, b2WorldDef b2WorldDefinition, const std::string& worldName = "");

			fs::PhysicsWorld* GetByName(const std::string& worldName);
			fs::PhysicsWorld* GetByID(uint8_t worldID);


			bool RemoveByName(const std::string& worldName);
			bool RemoveByID(uint8_t worldID);

		private:
			friend class fs::Runtime;

			bool Erase(uint8_t worldID);

			void Step();
		private:
			fs::IDAllocator m_worldAllocator;

			std::unordered_map<uint8_t, fs::PhysicsWorld> m_worlds;
			std::unordered_map<std::string, fs::PhysicsWorld*> m_worldsByName;
		};
	}

	inline fs_priv::PhysicsSingleplayerWorldManager singleplayerWorldManager;
}