#pragma once

#include <unordered_map>

#include "Input Common.hpp"

#include "render/Camera.hpp"

#include "glm/vec2.hpp"

namespace fs {
	namespace fs_priv {
		class Mouse {
		public:
			Mouse()  = default;
			~Mouse() = default;

			void UpdateScrollDelta(float delta);

			void ButtonUp(uint8_t button);
			void ButtonDown(uint8_t button);

			void UpdateButtons();

			float GetScrollDelta() const;

			InputState GetState(Button button);

			glm::vec2 GetNotRelativePosition();
			glm::vec2 GetPosition();
			glm::vec2 GetPosition(const Camera& camera);

		private:
			std::unordered_map<uint8_t, InputState> m_ButtonStates;

			float m_scrollDelta = 0.0f;

			InputState& GetKeyInputState(uint8_t button);

			static uint8_t GetSDLButton(Button button);
		};
	}

	inline fs_priv::Mouse Mouse;
}
