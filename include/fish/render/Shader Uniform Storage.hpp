#pragma once

#include <unordered_map>

#include "glm/mat4x4.hpp"

struct SDL_GPUCommandBuffer;

namespace fs {
	class ShaderUniformStorage {
	public:
		ShaderUniformStorage() = default;
		virtual ~ShaderUniformStorage() = default;

		template<typename T>
		void SetFragmentUniform(uint32_t binding, T value) {
			PushTempStorage(binding, value, m_FragmentTempUniformStorage);
		}

		template<typename T>
		void SetVertexUniform(uint32_t binding, T value) {
			PushTempStorage(binding, value, m_VertexTempUniformStorage);
		}

		void SetFragmentUniform(uint32_t binding, void* data, uint32_t size);
		void SetVertexUniform(uint32_t binding, void* data, uint32_t size);

		void UpdateRenderPassUniforms(SDL_GPUCommandBuffer* commandBuffer = nullptr);

	private:
		std::unordered_map<uint32_t, std::vector<uint8_t>> m_VertexTempUniformStorage;
		std::unordered_map<uint32_t, std::vector<uint8_t>> m_FragmentTempUniformStorage;

		bool m_storageEmpty = true;

		template<typename T>
		void PushTempStorage(uint32_t binding, T value, std::unordered_map<uint32_t, std::vector<uint8_t>>& Storage) {
			std::vector<uint8_t>& storageData = Storage[binding];
			
			uint64_t currentSize = storageData.size();
			uint32_t valueSize   = sizeof(T);
			uint64_t newSize     = currentSize + valueSize;

			storageData.resize(newSize);
			memcpy(&storageData[currentSize], &value, valueSize);

			m_storageEmpty = false;
		}

		void PushTempStorage(uint32_t binding, void* data, uint32_t dataSize, std::unordered_map<uint32_t, std::vector<uint8_t>>& Storage) {
			std::vector<uint8_t>& storageData = Storage[binding];

			uint64_t currentSize = storageData.size();
			uint32_t valueSize = dataSize;
			uint64_t newSize = currentSize + valueSize;

			storageData.resize(newSize);
			memcpy(&storageData[currentSize], data, valueSize);

			m_storageEmpty = false;
		}

		void PushFragmentUniform(SDL_GPUCommandBuffer* cmd, uint32_t binding, void* data, uint32_t size);
		void PushVertexUniform(SDL_GPUCommandBuffer* cmd, uint32_t binding, void* data, uint32_t size);
	};
}
