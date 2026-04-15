#pragma once

#include <unordered_map>

#include "Input Common.hpp"

#include "sdl3/SDL_scancode.h"

namespace fs {
	namespace fs_priv {
		class Keyboard {
		public:
			Keyboard()  = default;
			~Keyboard() = default;

			void KeyUp(SDL_Scancode code);
			void KeyDown(SDL_Scancode code);

			void UpdateKeys();

			InputState GetState(Key key);

		private:
			std::unordered_map<SDL_Scancode, InputState> m_KeyStates;

			InputState& GetKeyInputState(SDL_Scancode code);

			static SDL_Scancode GetSDLScanCode(Key key);
		};
	}

	inline fs_priv::Keyboard Keyboard;
}
