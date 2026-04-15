#pragma once
#include <box2d/math_functions.h>
#include <box2d/types.h>

#include "physics/PhysicsNetworkingData.hpp"

namespace boost {
	namespace serialization {

		// Vec2
		template<class Archive>
		void serialize(Archive& ar, b2Vec2& vec2, const unsigned int) {
			ar& vec2.x;
			ar& vec2.y;
		}

		// Rot
		template<class Archive>
		void serialize(Archive& ar, b2Rot& rot, const unsigned int) {
			ar& rot.s;
			ar& rot.c;
		}

		// Enum as uint8
		template<class Archive>
		void serialize(Archive& ar, b2BodyType& bodyType, const unsigned int) {
			ar& reinterpret_cast<uint8_t&>(bodyType);
		}

		// b2Polygon
		template<class Archive>
		void serialize(Archive& ar, b2Polygon& polygon, const unsigned int) {
			for (int i = 0; i < B2_MAX_POLYGON_VERTICES; ++i) {
				ar& polygon.vertices[i];
				ar& polygon.normals[i];
			}
			ar& polygon.centroid;
			ar& polygon.radius;
			ar& polygon.count;
		}

		// b2Filter -> from b2ShapeDef 
		template<class Archive>
		void serialize(Archive& ar, b2Filter& filter, const unsigned int) {
			ar& filter.categoryBits;
			ar& filter.maskBits;
			ar& filter.groupIndex;
		}

		// b2SurfaceMaterial -> from b2ShapeDef 
		template<class Archive>
		void serialize(Archive& ar, b2SurfaceMaterial& material, const unsigned int /*version*/) {
			ar& material.friction;
			ar& material.restitution;
			ar& material.rollingResistance;
			ar& material.tangentSpeed;
			ar& material.userMaterialId;
			ar& material.customColor;
		}

		// b2ShapeDef 
		template<class Archive>
		void serialize(Archive& ar, b2ShapeDef& shape, const unsigned int) {
			ar& shape.material;
			ar& shape.density;
			ar& shape.filter;
			ar& shape.isSensor;

			ar& shape.enableSensorEvents;
			ar& shape.enableContactEvents;
			ar& shape.enableHitEvents;
			ar& shape.enablePreSolveEvents;
			ar& shape.invokeContactCreation;
			ar& shape.updateBodyMass;

			// skip s.userData
			// skip s.internalValue
		}
		template<class Archive>
		void serialize(Archive& ar, b2BodyId& id, const unsigned int) {
			ar& id.index1;
			ar& id.world0;
			ar& id.generation;
		}
		template<class Archive>
		void serialize(Archive& ar, b2BodyDef& bodyDef, const unsigned int)
		{
			ar& bodyDef.type;
			ar& bodyDef.position;
			ar& bodyDef.rotation;
			ar& bodyDef.linearVelocity;
			ar& bodyDef.angularVelocity;
			ar& bodyDef.linearDamping;
			ar& bodyDef.angularDamping;
			ar& bodyDef.gravityScale;
			ar& bodyDef.sleepThreshold;

			ar& bodyDef.enableSleep;
			ar& bodyDef.isAwake;
			ar& bodyDef.fixedRotation;
			ar& bodyDef.isBullet;
			ar& bodyDef.isEnabled;
			ar& bodyDef.allowFastRotation;
		}
		template<class Archive>
		void serialize(Archive& ar, fs::PhysicsClientInputMoveDesire& clientInput, const unsigned int)
		{
			uint8_t clientRaw = static_cast<uint8_t>(clientInput);
			ar& clientRaw;
			if constexpr (!Archive::is_saving::value) {
				ar& static_cast<PhysicsClientInput>(clientRaw);
			}
		}
	}
}