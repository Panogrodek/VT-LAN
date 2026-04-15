#include "glm/vec2.hpp"

namespace fs {
	struct TextureRect {
		TextureRect() = default;
		TextureRect(glm::ivec2 size, glm::ivec2 position)
			: Size(size)
			, Position(position)
		{
		}

		glm::ivec2 Size		= glm::ivec2(0, 0);
		glm::ivec2 Position = glm::ivec2(0, 0);
	};
}
