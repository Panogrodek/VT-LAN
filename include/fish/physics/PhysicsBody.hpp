#pragma once
#include <box2d/types.h>
#include <boost/serialization/access.hpp>
#include <networking/ReceiverFlag.hpp>

#define INVALID_PHYSICS_ID 0 //you shouldn't change that

namespace fs {

	namespace fs_priv {
		class PhysicsWorldManager;
		class PhysicsMultiplayerWorldManager;
		class PhysicsNetworkingServer;
		class PhysicsNetworkingClient;
	}
	class PhysicsWorld;

	struct Box2dShapeData {
		Box2dShapeData() = default;
		~Box2dShapeData() = default;

		Box2dShapeData(const b2BodyDef& _body, const b2Polygon& _polygon, const b2ShapeDef& _shapeDef) :
			bodyDef(_body), polygon(_polygon), shapeDef(_shapeDef) {};

		b2BodyDef bodyDef;
		b2Polygon polygon;
		b2ShapeDef shapeDef;
		b2BodyId bodyID{};
	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version) {
			ar& bodyDef;
			ar& polygon;
			ar& shapeDef;
			ar& bodyID;
		}
	};



	class PhysicsBody {
	public:
		PhysicsBody() = default;
		PhysicsBody(const b2BodyDef& body, const b2Polygon& polygon, const b2ShapeDef& shapeDef, const std::string& customName = "");
		PhysicsBody(const Box2dShapeData& shapeData, const std::string& customName = "");

		~PhysicsBody() = default;

		PhysicsBody(const PhysicsBody&);
		PhysicsBody& operator=(const PhysicsBody&);

		PhysicsBody(PhysicsBody&&) noexcept = default;
		PhysicsBody& operator=(PhysicsBody&&) noexcept = default;



		bool IsInitialized() const;
		bool IsInserted() const;

		uint32_t GetCustomId() const;
		b2BodyId GetB2Id() const;
		const std::string& GetName() const;

		const b2Transform& GetTransform() const;
		const Box2dShapeData& GetShapeData() const;

		void SetUpdateAuthorities(ReceiverFlag authorities);
		const ReceiverFlag& GetUpdateAuthorities() const;
	private:
		friend class PhysicsWorld;
		friend class fs_priv::PhysicsWorldManager;
		friend class fs_priv::PhysicsMultiplayerWorldManager;
		friend class fs_priv::PhysicsNetworkingClient;
		friend class fs_priv::PhysicsNetworkingServer;

		//Who can modify this body using physics script (Ignored in singleplayer and bodies with player access)
		ReceiverFlag m_updateAuthorities = ReceiverFlag::Invalid;
		
		Box2dShapeData m_data;
		std::string m_name = "";
		uint32_t m_customId = 0; //0 for invalid
		bool m_initialized = false;
		bool m_inserted = false;

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version) {
			ar& m_data;
			ar& m_name;
			ar& m_customId;
			ar& m_updateAuthorities;
		}
	};
}