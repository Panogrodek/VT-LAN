#pragma once

#include <initializer_list>
#include <map>

#include "graphics/Texture.hpp"
#include "graphics/Shader Storage Buffer.hpp"

#include "glm/vec3.hpp"

#include "sdl3/SDL_gpu.h" //FIX ME!: this should not be in here but i can't move it because vector 
					      //has to know the sizes of the sdl structs, change to raw pointers maybe?

struct SDL_GPUComputePipeline;
struct SDL_GPUComputePass;
struct SDL_GPUCommandBuffer;

struct SDL_GPUTexture;
struct SDL_GPUTextureSamplerBinding;
struct SDL_GPUStorageTextureReadWriteBinding;

struct SDL_GPUBuffer;
struct SDL_GPUStorageBufferReadWriteBinding;

namespace fs {
	namespace fs_priv {
		struct ComputeTextureBinding {
			ComputeTextureBinding() = default;
			ComputeTextureBinding(class Texture& texture, uint32_t binding)
				: Texture(&texture)
				, Binding(binding)
			{ }

			class Texture* Texture = nullptr;
			uint32_t	   Binding = 0;
		};

		struct ComputeBufferBinding {
			ComputeBufferBinding() = default;
			ComputeBufferBinding(ShaderStorageBuffer& storageBuffer, uint32_t binding)
				: StorageBuffer(&storageBuffer)
				, Binding(binding)
			{ }

			ShaderStorageBuffer* StorageBuffer = nullptr;
			uint32_t			 Binding	   = 0;
		};

		class ComputeShaderUniformStorage {
		public:
			ComputeShaderUniformStorage() = default;
			virtual ~ComputeShaderUniformStorage() = default;

			void AddBuffer(Texture& texture, uint32_t binding);
			void AddBuffer(ComputeTextureBinding textureBinding);
			void AddBuffer(std::initializer_list<ComputeTextureBinding> TextureBindings);

			void AddBuffer(ShaderStorageBuffer& storageBuffer, uint32_t binding);
			void AddBuffer(ComputeBufferBinding storageBinding);
			void AddBuffer(std::initializer_list<ComputeBufferBinding> StorageBindings);

			template<typename T>
			void SetUniform(uint32_t binding, T value) {
				PushTempStorage(binding, value);
			}

			void SetUniform(uint32_t binding, void* data, uint32_t size);

		protected:
			std::vector<SDL_GPUTextureSamplerBinding>& GetSamplerTextureBindings();
			std::vector<SDL_GPUTexture*>& GetReadOnlyTextureBindings();
			std::vector<SDL_GPUStorageTextureReadWriteBinding>& GetReadWriteTextureBindings();

			std::vector<SDL_GPUBuffer*>& GetReadOnlyBufferBindings();
			std::vector<SDL_GPUStorageBufferReadWriteBinding>& GetReadWriteBufferBindings();

			void BuildSDLTextureBindings();
			void BuildSDLStorageBindings();
			void UpdateComputePassUniforms(SDL_GPUCommandBuffer* cmdBuffer);

		private:
			std::map<uint32_t, ComputeTextureBinding> m_TextureBindings;
			std::map<uint32_t, ComputeTextureBinding> m_ReadOnlyTextureBindings;
			std::map<uint32_t, ComputeTextureBinding> m_ReadWriteTextureBindings;

			std::map<uint32_t, ComputeBufferBinding> m_ReadOnlyBufferBindings;
			std::map<uint32_t, ComputeBufferBinding> m_ReadWriteBufferBindings;

			std::vector<SDL_GPUTextureSamplerBinding>		   m_TextureSamplerSDLBindings;
			std::vector<SDL_GPUTexture*>					   m_ReadOnlyTextureSDLBindings;
			std::vector<SDL_GPUStorageTextureReadWriteBinding> m_ReadWriteTextureSDLBindings;

			std::vector<SDL_GPUBuffer*> m_ReadOnlyBufferSDLBindings;
			std::vector<SDL_GPUStorageBufferReadWriteBinding> m_ReadWriteBufferSDLBindings;

			std::unordered_map<uint32_t, std::vector<uint8_t>> m_TempUniformStorage;

			bool m_textureBindingsChange = true;
			bool m_storageBindingsChange = true;
			bool m_uniformChange		 = true;

			template<typename T>
			void PushTempStorage(uint32_t binding, T value) {
				std::vector<uint8_t>& storageData = m_TempUniformStorage[binding];

				uint64_t currentSize = storageData.size();
				uint32_t valueSize   = sizeof(T);
				uint64_t newSize	 = currentSize + valueSize;

				storageData.resize(newSize);
				memcpy(&storageData[currentSize], &value, valueSize);

				m_uniformChange = true;
			}

			bool CheckTextureUsage(Texture& texture) const;
		};
	}

	class ComputeShader : public fs_priv::ComputeShaderUniformStorage {
	public:
		ComputeShader() = default;
		~ComputeShader();

		ComputeShader(const ComputeShader& other)  = delete;
		ComputeShader(const ComputeShader&& other) = delete;

		ComputeShader& operator=(const ComputeShader& other)  = delete;
		ComputeShader& operator=(const ComputeShader&& other) = delete;

		// this function should be called only when you finished all of texture binding, 
		// storage buffer binding, uniform changes etc, otherwise the result could be undefined 
		void DispatchCompute(uint32_t threadX = 1, uint32_t threadY = 1, uint32_t threadZ = 1, SDL_GPUCommandBuffer* commandBuffer = nullptr);
		void DispatchCompute(glm::uvec3 threadSize, SDL_GPUCommandBuffer* commandBuffer = nullptr);

		bool LoadFromFile(const std::string& path);
		bool LoadFromMemory(std::string shader, const std::string& cacheName);

		SDL_GPUComputePipeline* GetHandle();

	private:
		SDL_GPUComputePipeline* m_handle	    = nullptr;
		SDL_GPUComputePass*		m_computePass   = nullptr;
		SDL_GPUCommandBuffer*	m_commandBuffer = nullptr;

		struct ShaderObjectCount {
			uint32_t   SamplerCount				  = 0;
			uint32_t   UniformBufferCount		  = 0;
			uint32_t   ReadOnlyStorageImageCount  = 0;
			uint32_t   ReadOnlyStorageBufferCount = 0;
			uint32_t   StorageImageCount		  = 0;
			uint32_t   StorageBufferCount		  = 0;
			glm::uvec3 ThreadCount				  = glm::uvec3(1);
		};

		bool CompileFromFile(const std::string& path);
		bool CompileFromMemory(std::string shader, const std::string& cacheName);

		bool BuildFromCacheFile(const std::string& path, bool checkModificationTime = true);

		void CreateShader(void* shaderSource, uint64_t size, ShaderObjectCount shaderObjectCount);

		ShaderObjectCount GetShaderObjectCount(void* source, uint64_t sourceSize);

		std::vector<uint32_t> CompileShader(void* source, uint64_t size, const std::string& fileName) const;
	};
}
