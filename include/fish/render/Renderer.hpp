#pragma once

#include <vector>
#include <unordered_map>
#include <utility>
#include <map>

#include "core/Runtime.hpp"

#include "graphics/Framebuffer.hpp"

#include "graphics/Vertex Buffer.hpp"
#include "graphics/Index Buffer.hpp"

#include "graphics/Texture.hpp"
#include "graphics/Uber Texture.hpp"

#include "graphics/Shader Storage Buffer.hpp"
#include "graphics/Shader.hpp"

#include "graphics/Vertex.hpp"

#include "graphics/Quad.hpp"
#include "graphics/Batch Quad.hpp"

#include "graphics/Text.hpp"

#include "render/Camera.hpp"
#include "render/Render States.hpp"
#include "render/Render Pipeline.hpp"
#include "glm/vec2.hpp"

struct SDL_GPURenderPass;
struct SDL_GPUTexture;
struct SDL_GPUCommandBuffer;

namespace fs {
	namespace fs_priv {
		struct PipelineUniformData {
			const Camera* Camera = nullptr;
			glm::mat4 Transform  = glm::mat4(1.0f);
		};

		struct TextureCache {
			const Texture* m_lastBoundAlbedoTx = nullptr;
			const Texture* m_lastBoundNormalTx = nullptr;
		};

		struct DrawCallCache {
			const Quad*			Quad		 = nullptr;
			const Text*			Text		 = nullptr;
			const Camera*		Camera		 = nullptr;
			const VertexBuffer* VertexBuffer = nullptr;
			const Vertex*		Vertices	 = nullptr;
			const uint32_t*		Indices		 = nullptr;

			RenderStates  States;
			uint64_t	  FirstVertex = 0;
			uint64_t	  VertexCount = 0;
			uint64_t	  FirstIndex  = 0;
			uint64_t	  IndexCount  = 0;

			Layer Layer = Layers::Default;

			DrawCallCache() = default;
			DrawCallCache(const fs::Quad* quad, const fs::Camera* camera, fs::Layer layer, RenderStates states);
			DrawCallCache(const fs::Text* text, const fs::Camera* camera, fs::Layer layer, RenderStates states);
			DrawCallCache(const fs::VertexBuffer* vertexBuffer, const fs::Camera* camera, fs::Layer layer, RenderStates states);
			DrawCallCache(const Vertex* Vertices, uint32_t vertexCount, const fs::Camera* camera, fs::Layer layer, RenderStates states);
			DrawCallCache(const Vertex* Vertices, uint32_t vertexCount, const uint32_t* Indices, uint32_t indexCount, const fs::Camera* camera, RenderStates states);
		};

		struct BatchDrawCallCache {
			const BatchQuad* BatchQuad = nullptr;
			const Camera*	 Camera	   = nullptr;
			uint64_t		 OrderFallback = 0;

			RenderStates States;

			Layer Layer = Layers::Default;

			BatchDrawCallCache(const fs::BatchQuad* quad, const fs::Camera* camera, fs::Layer layer, uint64_t orderFallback, RenderStates states);
		};

		struct BatchUniformData {
			glm::mat4 Projection;
			glm::mat4 View;
			glm::mat4 Model;
		};

		struct DrawTargetCache {
			TextureCache StandardTextureCache;
			TextureCache UberTextureCache;

			uint64_t BatchDrawOrderFallback = 0;

			std::vector<DrawCallCache> CachedDrawCalls;
			std::unordered_map<uint64_t, std::vector<BatchDrawCallCache>> BatchCachedDrawCalls;
		};

		struct BatchDrawData {
			uint32_t FirstVertex = 0;
			uint32_t VertexCount = 0;

			UberTexture* AlbeodTx = nullptr;
			UberTexture* NormalTx = nullptr;

			Layer Layer = Layers::Default;

			const Camera* Camera = nullptr;
		};

		struct RenderPass {
			std::unordered_map<uint64_t, BatchDrawData> BatchDrawData;
			std::vector<DrawCallCache> StandardDrawData;
		};

		class Renderer {
		public:
			Renderer()  = default;
			~Renderer() = default;

			void SetVSync(bool enabled);

			void ClearBuffers();
			void BuildSwapchainCommandBuffer();
			void DrawDataToBuffers();
			void Flush();

			void Draw(const Quad& quad, RenderStates states = DefaultStates);
			void Draw(const BatchQuad& quad, RenderStates states = DefaultStates);
			void Draw(const Text& text, RenderStates states = DefaultStates);
			//void Draw(const VertexBuffer& vertexBuffer, RenderStates states = DefaultStates);
			//void Draw(const Vertex* Vertices, uint32_t vertexCount, RenderStates states = DefaultStates);
			void Draw(const Vertex* Vertices, uint32_t vertexCount, const uint32_t* Indices, uint32_t indexCount, RenderStates states = DefaultStates);

			void AttachFramebuffer(Framebuffer& framebuffer);
			void DeattachFrameBuffer();

			void Reinitialize(glm::vec2 resolution);

			Texture& GetMissingTx();

			glm::vec2 GetResolution() const;

			PipelineData GetDefaultPipelineData() const;

			SDL_GPUTexture* GetSwapchainTexture();
			SDL_GPURenderPass* GetFlushRenderPass();
			SDL_GPUCommandBuffer* GetSwapchainCommandBuffer();

		private:
			glm::vec2 m_renderResolution = glm::vec2(1.0f, 1.0f);

			std::unordered_map<Framebuffer*, DrawTargetCache> m_CachedDrawCalls;

			std::map<Layer, RenderPass> m_RenderPasses;

			PipelineUniformData m_pipelineUniformData;

			std::vector<BatchUniformData> m_batchUniformData;
			std::vector<BatchVertex>	  m_batchVertices;
			std::vector<uint32_t>		  m_batchIndices;

			std::vector<Vertex>	  m_defaultVertices;
			std::vector<uint32_t> m_defaultIndices;

			RenderPipeline m_defaultLightPipeline;
			RenderPipeline m_batchLightPipeline;

			RenderPipeline m_defaultPipeline;
			RenderPipeline m_batchPipeline;
			RenderPipeline m_flushPipeline;

			Framebuffer* m_attachedFramebuffer = nullptr;
			Framebuffer  m_defaultFramebuffer;

			Shader m_defaultLightShader;
			Shader m_batchLightShader;

			Shader m_defaultShader;
			Shader m_batchShader;

			IndexBuffer m_batchIndexBuffer;
			IndexBuffer m_defaultIndexBuffer;

			VertexBuffer m_flushVeretxBuffer;
			VertexBuffer m_defaultVertexBuffer;
			VertexBuffer m_batchVertexBuffer;

			ShaderStorageBuffer m_batchUniformBuffer;

			Texture m_missingTx;
			Texture m_defaultAlbedoTx;
			Texture m_defaultNormalTx;

			Texture* m_activeBatchAlbedoTx = nullptr;
			Texture* m_activeBatchNormalTx = nullptr;

			RenderPipeline*		  m_currentPipeline   = nullptr;
			SDL_GPURenderPass*	  m_currentRenderPass = nullptr;
			SDL_GPUCommandBuffer* m_passCommandBuffer = nullptr;

			SDL_GPUTexture*		  m_swapchainTexture	   = nullptr;
			SDL_GPURenderPass*    m_flushRenderPass		   = nullptr;
			SDL_GPUCommandBuffer* m_swapchainCommandBuffer = nullptr;

			void DrawBatchQuads(BatchDrawData& drawCall, TextureCache& textureCache);
			void DrawQuad(DrawCallCache& drawCall, TextureCache& textureCache);
			void DrawTextSequence(DrawCallCache& drawCall);
			void DrawVerticesWithIndices(DrawCallCache& drawCall, TextureCache& textureCache);

			template<typename ...Args>
			DrawTargetCache& FeedDrawTargetCache(Args&&... args) {
				Framebuffer* framebuffer = m_attachedFramebuffer;

				if (!framebuffer)
					framebuffer = &m_defaultFramebuffer;

				DrawTargetCache& drawTargetCache = m_CachedDrawCalls[framebuffer];

				drawTargetCache.CachedDrawCalls.emplace_back(std::forward<Args>(args)...);
				return drawTargetCache;
			}

			void RenderCachedData(Framebuffer& framebuffer);

			void FeedPipeline(RenderPipeline& pipeline);
			void FeedPipelineLightData(RenderPipeline& pipeline, const Camera* camera, Layer layer);
			void FeedPipelineTextures(SDL_GPURenderPass* renderPass, TextureCache& textureCache, const Texture* albedoTexture, const Texture* normalTexture, const RenderStates& states);
			void FeedBatchPipelineTextures(SDL_GPURenderPass* renderPass, TextureCache& textureCache, const UberTexture* albedoTexture, const UberTexture* normalTexture);

			void FeedStandardVertexData(DrawTargetCache& drawTargetCache);
			void FeedBatchVertexData(DrawTargetCache& drawTargetCache);

			std::pair<uint32_t, uint32_t> FeedQuadVertices(DrawCallCache& call, uint32_t firstVertex);
			std::pair<uint32_t, uint32_t> FeedTextVertices(DrawCallCache& call, uint32_t firstVertex, uint32_t firstIndex);
			std::pair<uint32_t, uint32_t> FeedStandardVerticesAndIndices(DrawCallCache& call, uint32_t firstVertex, uint32_t firstIndex);

			void ClearPipelineUniformData();

			void Initialize(glm::vec2 resolution);

			void LoadShaders();
			void BuildPipelines();
			void BuildFramebuffers();
			void BuildTextures();
			void BuildVertexBuffers();

			friend bool Runtime::Initialize();
		};
	}

	inline fs_priv::Renderer Renderer;
}
