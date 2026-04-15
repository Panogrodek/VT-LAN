#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace fs {
	struct Vertex {
		Vertex
		(
			glm::vec3 position  = glm::vec3(0.0f),
			glm::vec2 texCoords = glm::vec2(0.0f),
			glm::vec4 color     = glm::vec4(1.0f)
		)
			: Position (position)
			, TexCoord (texCoords)
			, Color    (color)
		{ }

		~Vertex() = default;

		glm::vec3 Position;
		glm::vec2 TexCoord;
		glm::vec4 Color;
	};

	struct BatchVertex {
		BatchVertex
		(
			glm::vec3 position		 = glm::vec3(0.0f),
			glm::vec2 texCoords		 = glm::vec2(0.0f),
			glm::vec2 normalTexCoord = glm::vec2(0.0f),
			glm::vec4 color			 = glm::vec4(1.0f),
			int		  index			 = 0
		)
			: Position(position)
			, TexCoord(texCoords)
			, NormalTexCoord(normalTexCoord)
			, Color(color)
			, Index(index)
		{
		}

		~BatchVertex() = default;

		glm::vec3 Position;
		glm::vec2 TexCoord;
		glm::vec2 NormalTexCoord;
		glm::vec4 Color;
		int		  Index = 0;
	};
}
