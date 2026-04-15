#pragma once

extern const long double& et;

extern const float& dt;
extern const float& ndt;
extern const float& pdt;

extern const bool& netUpdt;

namespace fs {
	struct TimeSteps {
		static const float MaxDeltaTime;

		static const float NetworkingSendStepTime;
		static const float NetworkGNSStepTime;
		
		static const float PhysicsStepTime;
		static const float VoiceReceiveStepTime;
		

		static const bool& NetworkingSendUpdate;
		static const float& NetworkSendDeltaTime;
		
		static const float& NetworkGNSDeltaTime;

		static const float& PhysicsDeltaTime;

		static const long double& ElapsedTime;
		static const float& DeltaTime;

	};
}
