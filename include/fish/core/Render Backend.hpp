#pragma once

#include "Runtime.hpp"

#include "graphics/Texture.hpp"

/*
	SDL_GPU wrapper class, creates GPU context (vulkan, metal, dx12 etc) and provides easy access to it.
*/

struct SDL_GPUDevice;
struct SDL_GPUTexture;
struct SDL_GPUSampler;
struct SDL_GPUFence;
struct SDL_GPUCommandBuffer;
struct SDL_GPUColorTargetInfo;
struct SDL_GPUDepthStencilTargetInfo;

struct TTF_TextEngine;

namespace fs {
	namespace fs_priv {
		class RenderBackend {
		public:
			SDL_GPUDevice* GetContext();
			const char* GetDriverName();

			TTF_TextEngine* GetTextContext();

			SDL_GPUCommandBuffer* BuildCommandBuffer();
			SDL_GPUTexture* BuildSwapchainTexture(SDL_GPUCommandBuffer* commandBuffer);

			void PushCommandBuffer(SDL_GPUCommandBuffer* commandBuffer);
			SDL_GPUFence* PushCommandBufferAndGetFence(SDL_GPUCommandBuffer* commandBuffer);

			SDL_GPURenderPass* BeginRenderPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUColorTargetInfo* colorTargetInfo, uint32_t colorTargetCount, SDL_GPUDepthStencilTargetInfo* depthStencilTargetInfo);
			void PushRenderPass(SDL_GPURenderPass* renderPass);

			void SetVSync(bool vsync = false);

			SDL_GPUSampler* GetSampler(SamplerType type);

			uint32_t GetShaderFormat() const;

		private:
			uint32_t m_shaderFormat = 0;

			SDL_GPUDevice*  m_context     = nullptr;
			TTF_TextEngine* m_textContext = nullptr;
			SDL_GPUSampler* m_Samplers[6] = { nullptr };

			void Initialize();
			void InitializeTextEngine();
			void BuildTextureSamplers();

			friend bool Runtime::Initialize();
		};
	}

	inline fs_priv::RenderBackend RenderBackend;
}
