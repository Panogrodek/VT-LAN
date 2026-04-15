#pragma once

#include <unordered_set>
#include <bitset>
#include <array>

#include "Input Common.hpp"

enum SDL_Scancode;

namespace fs {
	template<typename Code, size_t N>
	struct DeviceLayer {
		std::array<InputState, N> States{};
		std::bitset<N>            Locks{};
		std::unordered_set<Code>  Pressed;
		std::unordered_set<Code>  Released;

		void Update() {
			// released -> none
			for (auto code : Released)
				States[size_t(code)] = InputState::None;

			// pressed -> held
			for (auto code : Pressed) {
				auto idx = size_t(code);
				if (States[idx] != InputState::Blocked)
					States[idx] = InputState::Held;
			}
			Released.clear();
			Pressed.clear();
		}

		void Press(Code code) {
			size_t idx = size_t(code);
			auto& st = States[idx];
			if (st == InputState::Pressed)
				st = InputState::Held;
			else if (st != InputState::Held && st != InputState::Blocked) {
				st = InputState::Pressed;
				Pressed.insert(code);
			}
		}

		void PermLock(Code code)		   { Locks.set(size_t(code));		  }
		void UnpermLock(Code code)		   { Locks.reset(size_t(code));		  }
		bool IsPermLocked(Code code) const { return Locks.test(size_t(code)); }

		void Release(Code code) {
			size_t idx = size_t(code);
			States[idx] = InputState::Released;
			Released.insert(code);
		}

		bool IsBlocked(Code code) const {
			return Locks.test(size_t(code));
		}

		InputState Get(Code code) {
			return States[size_t(code)];
		}
	};

	class InputLayer {
	public:
		InputLayer()  = default;
		~InputLayer() = default;

		void Update();

		void KeyUp(SDL_Scancode code);
		void KeyDown(SDL_Scancode code);

		void ButtonUp(uint8_t buttonCode);
		void ButtonDown(uint8_t buttonCode);

		void LockInput();
		void UnlockInput();

		// Permanently locks key so the next update loop wont 
		// unlock it, the key can be ulocked by calling Unlock()
		void Permalock(); // Locks last checked key

		void Permalock(Key key);
		void Permalock(Button button);

		void Unlock(Key key);
		void Unlock(Button button);

		// Locks last checked key
		void Lock();

		void Lock(Key key);
		void Lock(Button button);

		InputState GetState(Key key);
		InputState GetState(Button button);

		InputState operator()(Key key);
		InputState operator()(Button button);

	private:
		bool m_inputBlocked = false;
		bool m_mouseLast	= false; // true if last input was from mouse

		Key	   m_lastKey	= Key::Unknown;
		Button m_lastButton = Button::Unknown;

		static constexpr size_t KEY_COUNT = size_t(Key::KeyCount);
		static constexpr size_t BUTTON_COUNT = size_t(Button::ButtonCount);

		DeviceLayer<Key, KEY_COUNT>		  m_keyboard;
		DeviceLayer<Button, BUTTON_COUNT> m_mouse;

		Button GetButtonFromSDL(uint8_t button) const;
		Key GetKeyFromSDL(SDL_Scancode button) const;
	};
}
