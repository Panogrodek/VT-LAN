#pragma once

#include <unordered_set>
#include <unordered_map>

#include "core/Runtime.hpp"

#include "render/light/Light.hpp"
#include "render/Camera.hpp"

#include "graphics/Compute Shader.hpp"
#include "graphics/Shader Storage Buffer.hpp"

/*
	Per-layer light system.

	On initialization, a fixed number of Layers is created.
	For each Layer, we preallocate an SSBO containing light cluster information.
	This increases memory usage but improves performance by avoiding per-object SSBO updates.

	Layers marked with the NO_LIGHT flag are excluded from light calculations.??????? delete this
*/

struct SDL_GPURenderPass;
struct SDL_GPUCommandBuffer;

namespace fs {
	namespace fs_priv {
		class LightSystem {
		public:
			LightSystem()  = default;
			~LightSystem() = default;

			void AddLight(Light* light);
			void RemoveLight(Light* light);

			void UpdateLights();

			void BindLightStorageBuffer(SDL_GPURenderPass* renderPass, uint32_t slot);
			void BindLightIndicesStorageBuffer(SDL_GPURenderPass* renderPass, uint32_t slot);

			void BindStorageBuffers(SDL_GPURenderPass* renderPass);

			uint32_t GetCameraLightIndex(const Camera& camera) const;
			uint32_t GetLightCount() const;

		private:
			static constexpr uint32_t MAX_LIGHT_COUNT = 128;

			SDL_GPUCommandBuffer* m_commandBuffer = nullptr;

			std::unordered_set<Light*> m_Lights;
			std::vector<LightData>	   m_LightData;

			std::vector<glm::vec4>	   m_CameraFrustumsMinMax;

			uint32_t m_lightCount = 0;

			std::unordered_map<const Camera*, uint32_t> m_CameraIndexMap;
			uint32_t m_currentCameraIndex = 0;

			ShaderStorageBuffer m_cameraFrustumsMinMaxStorageBuffer;
			ShaderStorageBuffer m_lightIndicesStorageBuffer;
			ShaderStorageBuffer m_lightStorageBuffer;

			ComputeShader m_lightCull;

			void BeginLightUpdate();

			void RunLightCull();

			void BuildCameraIndexMap();
			void UpdateCameraFrustumStorage();

			glm::vec4 CalculateCameraMinMax(const Camera& camera);

			void UpdateLightStorage();
			void Initialize();

			friend bool Runtime::Initialize();
		};
	}

	inline fs_priv::LightSystem LightSystem;
}
