#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <set>

#include "glm/vec2.hpp"

struct SDL_Window;
union  SDL_Event;

namespace fs {
	namespace fs_priv {
		class Window;
	}

	enum class WindowType {
		Windowed,
		FullscreenWindowed
	};

	enum class WindowMode {
		None,
		Decorated,
		Closable
	};

	enum class AspectRatio {
		Ratio_16_9 = 0,
		Ratio_4_3,
		Ratio_16_10
	};

	struct WindowData {
		const char* Title = "Fusion";
		WindowType  WindowType = WindowType::Windowed;
		WindowMode  WindowMode = WindowMode::Decorated;
		glm::uvec2  Size = glm::uvec2(640, 640);
	};

	struct DisplayMode {
		glm::uvec2 Size = glm::uvec2(0);
		float      RefreshRate = 0;

		bool operator==(const DisplayMode& other) const {
			return Size == other.Size;
		}

		// warning this operator is used only for set sorting purposes! its reversed
		bool operator<(const DisplayMode& other) const {
			return Size.x > other.Size.x;
		}
	};

	struct Display {
		const std::string& GetName() const { return m_name; }
		
		glm::uvec2 GetDesktopSize() const { return m_desktopSize; }

		int GetHandle() const { return m_handle; }

		bool operator==(const Display& other) const {
			return m_handle == other.m_handle;
		}

	private:
		std::string m_name = "";
		glm::uvec2  m_desktopSize = glm::uvec2(0);
		int	m_handle = -1;

		friend class fs::fs_priv::Window;
	};

}

// hash function for display
namespace std {
	template<>
	struct hash<fs::Display> {
		std::size_t operator()(const fs::Display& d) const noexcept {
			return std::hash<uint32_t>()(d.GetHandle());
		}
	};
}

namespace std {
	template<>
	struct hash<fs::DisplayMode> {
		std::size_t operator()(const fs::DisplayMode& dm) const noexcept {
			std::size_t seed = 64007; // groovy seed
			HashCombine(seed, std::hash<unsigned int>{}(dm.Size.x));
			HashCombine(seed, std::hash<unsigned int>{}(dm.Size.y));
			return seed;
		}

	private:
		static void HashCombine(std::size_t& seed, std::size_t value) noexcept {
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
	};
}

namespace fs{
	namespace fs_priv {
		class Window {
		public:
			Window()  = default;
			~Window() = default;

			bool Create(WindowData data);

			void SetDisplay(Display display);

			void SetWindowType(WindowType type);

			bool PollEvent(SDL_Event* event);

			void SetSize(glm::uvec2 size);

			bool IsFullscreen() const;

			SDL_Window* GetHandle();

			glm::uvec2 GetSize();
			glm::ivec2 GetPosition();

			Display GetPrimaryDisplay();
			Display GetCurrentDisplay();

			const std::vector<Display>& GetDisplays() const;
			const std::set<DisplayMode>& GetDisplayModes(Display display, AspectRatio aspectRatio) const;

		private:
			SDL_Window* m_window = nullptr;

			bool m_fullscreen = false;

			WindowType m_type = WindowType::Windowed;

			Display m_currentDisplay;
			Display m_primaryDisplay;

			std::vector<Display> m_Displays;
			std::unordered_map<Display, std::unordered_map<AspectRatio, std::set<DisplayMode>>> m_DisplayModes; // a bit cursed but works

			uint64_t GetWindowType(WindowType type) const;
			uint64_t GetWindowMode(WindowMode mode) const;

			bool InitializeDisplays();
		};
	}

	inline fs_priv::Window Window;
}
