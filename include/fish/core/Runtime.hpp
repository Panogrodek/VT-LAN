#pragma once

#include <functional>

#include "core/Time Steps.hpp"

#include "system/Window.hpp"
#include "system/Clock.hpp"


//TODO: this framework still crashes when you launch it without steam present on your PC (obv. the non steam gns version, but it should only crash/throw error when you initialize
//steam without steam present. There are steam api functions for that im just lazy af

union SDL_Event;

namespace fs {
	class Runtime {
	public:
		Runtime();
		~Runtime() = default;

		static bool Initialize();															   // This functions are bool type because on dist 
		static bool Initialize(WindowData data, glm::vec2 renderResolution = glm::vec2(0.0f)); // they should throw false to do proper error handling 
																							   // /then they should throw some kind of error code imo
		static void Shutdown();

		void Run();

	protected:
		virtual void Update() = 0;
		virtual void Render() = 0;
		virtual void PhysicsUpdate() = 0;

		void SetExternalRender(bool externalRender);
		void SetEventCallback(std::function<void(SDL_Event)> callback); //FIX ME: ? its a hacky way of doing things but we dont have event system abstracted yet

	private:
		static Runtime* s_Instance;

		static glm::vec2 s_initialResolution;


		static long double s_elapsedTime;
		static float s_deltaTime;

		static float s_networkingSendDeltaTime;
		static bool s_networkingSendUpdate;

		static float s_networkingGNSDeltaTime;

		static float s_physicsDeltaTime;
		static float s_physicsAccumulator;


		bool m_running = true;
		bool m_externalRender = false;

		Clock m_deltaClock;
		int64_t m_lastNetPoll = 0;

		std::function<void(SDL_Event)> m_eventCallback = 0;

		void UpdatePass();
		void HandleEvents();
		void RenderPass();
		void FixedTimestepPass();

		void NetworkUpdateStart();
		void NetworkUpdateEnd();

		friend struct TimeSteps;
	};
}
