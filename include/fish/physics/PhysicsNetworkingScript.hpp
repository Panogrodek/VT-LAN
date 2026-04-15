#pragma once
#include "PhysicsBody.hpp"
#include "PhysicsNetworkingData.hpp"

#include <functional>

namespace fs {
	class PhysicsNetworkingWorld;

	class PhysicsNetworkingScript {
	public:
		using UserFunction = std::function<void(const PhysicsBody*,PhysicsClientInputData)>;
		
		void SetUserFunction(UserFunction func);
		void ClearUserFunction();

	private:
		void Step(PhysicsClientInputData data, bool asServer = false, bool ignoreAuthority = false) const;
		void StepClientReconciliation(const PhysicsClientInputData& data) const;
	private:
		friend class fs_priv::PhysicsNetworkingServer;
		friend class fs_priv::PhysicsNetworkingClient;
		friend class PhysicsNetworkingWorld;

		uint16_t m_scriptID = INVALID_PHYSICS_ID;
		uint8_t m_boundWorldID = INVALID_PHYSICS_ID;

		UserFunction m_userFunc = nullptr;
	};
}