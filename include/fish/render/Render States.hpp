#pragma once

#include "render/Layer.hpp"

#include "glm/mat4x4.hpp"

namespace fs {
	class Texture;
	class RenderPipeline;

	struct RenderStates {
		RenderStates() = default;

		RenderStates(RenderPipeline* pipeline)
			: RenderPipeline(pipeline)
		{ }

		glm::mat4 Transform = glm::mat4(1.0f);

		Texture* AlbedoTx = nullptr;
		Texture* NormalTx = nullptr;

		RenderPipeline* RenderPipeline = nullptr; // FOR NOW WORKS ONLY FOR fs::Quad! This setting should not be used/switched often it can cause lag spikes

		Layer Layer = Layers::Default;

		bool FeedTextures  = true;
		bool FeedLightData = true;
		bool FeedPipelineUniforms = true;
	};

	inline RenderStates DefaultStates;
}
