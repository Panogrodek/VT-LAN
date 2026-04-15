#pragma once
#include "PhysicsBody.hpp"

namespace fs {
	enum class PhysicsClientInputMoveDesire : uint8_t {
		None	= 0,
		Up		= 1 << 0,
		Down	= 1 << 1,
		Left	= 1 << 2,
		Right	= 1 << 3,
		UndefinedData5 = 1 << 4,
		UndefinedData6 = 1 << 5,
		UndefinedData7 = 1 << 6,
		UndefinedData8 = 1 << 7,
	};
	//operators
	inline fs::PhysicsClientInputMoveDesire operator|(fs::PhysicsClientInputMoveDesire lhs, fs::PhysicsClientInputMoveDesire rhs)
	{
		return static_cast<fs::PhysicsClientInputMoveDesire>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
	}
	inline fs::PhysicsClientInputMoveDesire& operator|=(fs::PhysicsClientInputMoveDesire& lhs, fs::PhysicsClientInputMoveDesire rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}



	struct PhysicsClientInputData {
		PhysicsClientInputMoveDesire moveDesire = PhysicsClientInputMoveDesire::None;
		int64_t userData{};

		uint32_t bodyID = INVALID_PHYSICS_ID;
	private:
		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar& userData;
			ar& moveDesire;
			ar& bodyID;
		}
	};



	struct PhysicsClientInputPacket {
		PhysicsClientInputData input{};

		uint8_t worldID = INVALID_PHYSICS_ID;

		uint32_t tick = UINT32_MAX;
	public:
		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar& input;
			ar& worldID;
			ar& tick;
		}
	};


	//TODO: optimize values
	struct PhysicsSyncPacket {
		PhysicsSyncPacket() = default;

		uint8_t worldID = INVALID_PHYSICS_ID;
		uint32_t bodyID = INVALID_PHYSICS_ID;

		uint32_t tick = UINT32_MAX; //Invalid tick id

		float posX = 0;
		float posY = 0;

		float rotCos = 0; //TODO: this can be optimised
		float rotSin = 0;

		float linVelX = 0;
		float linVelY = 0;

		float angVel = 0;
	private:
		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar& worldID;
			ar& bodyID;
			ar& tick;
			ar& posX;
			ar& posY;
			ar& rotCos;
			ar& rotSin;
			ar& linVelX;
			ar& linVelY;
			ar& angVel;
		}
	};



	struct PhysicsCreatePacket {
		PhysicsCreatePacket() = default;

		fs::PhysicsBody body{};
		uint8_t worldID = INVALID_PHYSICS_ID;
		uint32_t tick = UINT32_MAX; //MAX for invalid
	private:
		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar& worldID;
			ar& body;
			ar& tick;
		}
	};



	struct PhysicsRemovePacket {
		PhysicsRemovePacket() = default;
		uint32_t bodyID = INVALID_PHYSICS_ID;
		uint8_t worldID = INVALID_PHYSICS_ID;
		
		uint32_t tick = UINT32_MAX;
	private:
		friend class boost::serialization::access;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar& worldID;
			ar& bodyID;
			ar& tick;
		}
	};

}