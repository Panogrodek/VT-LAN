#pragma once

#include "GLM/vec4.hpp"
#include "GLM/vec3.hpp"

namespace fs {
	enum class LightType {
		Invalid = 0,
		PointLight,
		SpotLight
	};

	constexpr uint32_t MAX_LIGHTS_PER_CAMERA = 64;

	struct alignas(16) LightIndices {
		uint32_t LightIndices[MAX_LIGHTS_PER_CAMERA];
		uint32_t LightCount = 0;
	};

	struct alignas(16) LightData {
		glm::vec4 Position  = glm::vec4(0.0f,  0.0f, 50.0f, 100.0f); // .z comp is light height used in normals .w comp is light radius.
		glm::vec4 Color     = glm::vec4(1.0f,  1.0f, 1.0f, 0.0f); // .w comp is light type.
		glm::vec4 Direction = glm::vec4(0.0f, -1.0f, 0.0f, 0.5f); // .w comp is cosinus of cutoff angle.
		glm::vec4 Data		= glm::vec4(0.0f,  1.0f, 0.0f, 0.0f); // .x comp is light layer, .y comp is fallthrough state
	};
}
