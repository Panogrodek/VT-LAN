#pragma once

#include <vector>
#include <unordered_map>

#include "graphics/Texture.hpp"
#include "graphics/Framebuffer.hpp"

#include "glm/vec2.hpp"

namespace fs {
	struct UberTextureRect {
		glm::ivec2 Position = glm::ivec2(0);
		glm::ivec2 Size		= glm::ivec2(0);
		Texture*   Texture  = nullptr;
	};

	namespace fs_priv {
		class UberTextureBuilder {
		public:
			UberTextureBuilder()  = default;
			~UberTextureBuilder() = default;

			void Create(std::vector<Texture*> Textures, std::unordered_map<Texture*, UberTextureRect>& outputRects, Texture& outputTexture);

		private:
			Framebuffer m_textureBuffer;

			int FindMinSquare(std::vector<UberTextureRect>& rects);
		};
	}

	inline fs_priv::UberTextureBuilder UberTextureBuilder;
}
