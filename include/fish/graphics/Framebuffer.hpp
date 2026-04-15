#pragma once

#include <unordered_map>
#include <string>

#include "graphics/Texture.hpp"
#include "render/Render Pipeline.hpp"
#include "glm/vec4.hpp"

struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUTexture;

namespace fs {
	class Framebuffer {
	public:
		Framebuffer()  = default;
		~Framebuffer() = default;

		Framebuffer(const Framebuffer& other)  = delete;
		Framebuffer(const Framebuffer&& other) = delete;

		Framebuffer& operator=(const Framebuffer& other)  = delete;
		Framebuffer& operator=(const Framebuffer&& other) = delete;

		void BeginDraw();
		void EndDraw();

		void SetPipeline(RenderPipeline* pipeline);
		void SetBatchPipeline(RenderPipeline* pipeline);

		RenderPipeline* GetPipeline();
		RenderPipeline* GetBatchPipeline();

		void SetClearColor(glm::vec4 clearColor);
		glm::vec4 GetClearColor();

		Texture& GetTexture(const std::string& name);

		void AddTexture(const std::string& name, TextureData data);
		void RemoveTexture(const std::string& name);

		void AddDepthTexture(glm::vec2 size);

		SDL_GPURenderPass* GetRenderPass();
		SDL_GPUCommandBuffer* GetCommandBuffer();

	private:
		glm::vec4 m_clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		SDL_GPURenderPass*    m_renderPass    = nullptr;
		SDL_GPUCommandBuffer* m_commandBuffer = nullptr;

		RenderPipeline* m_pipeline		= nullptr;
		RenderPipeline* m_batchPipeline = nullptr;

		std::unordered_map<std::string, Texture> m_ColorTargets;
		std::vector<std::string>				 m_ColorTargetOrder;
	};
}
