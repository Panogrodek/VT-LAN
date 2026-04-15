#pragma once

#include "graphics/Quad.hpp"

#include "glm/vec2.hpp"

namespace fs {
	struct Animation {
		Animation()  = default;
		~Animation();

		Animation(const Animation& other);
		Animation(const Animation&& other) noexcept;

		Animation& operator=(const Animation& other);
		Animation& operator=(const Animation&& other) noexcept;

		float ElapsedTime = 0.0f;
		float FrameTime   = 1.0f;
		
		bool Loop     = true;
		bool Reversed = false;
		bool Running  = true;

		glm::uvec2 Size		   = glm::uvec2(0u);
		glm::ivec2 FrameOffset = glm::ivec2(0);

		uint32_t FramesX = 1;
		uint32_t FramesY = 1;

		uint32_t FrameCount	  = 0;
		int		 CurrentFrame = 0;

		TextureRect* Frames = nullptr;

		Quad*	 Body	 = nullptr;
		Texture* Texture = nullptr;

		void Create(uint32_t frameCount);
		void SetFrame(uint32_t frame);

	private:
		void Copy(const Animation& other);
	};
}
