#pragma once

#include <cstdint>

struct SDL_GPUBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUCommandBuffer;

namespace fs {
	class VertexBuffer {
	public:
		VertexBuffer() = default;
		~VertexBuffer();

		VertexBuffer(const VertexBuffer& other);
		VertexBuffer(const VertexBuffer&& other) noexcept;

		VertexBuffer& operator=(const VertexBuffer& other);
		VertexBuffer& operator=(const VertexBuffer&& other) noexcept;

		void Create(uint64_t size);
		void Create(void* data, uint64_t size);
		void Update(void* data, uint64_t size, SDL_GPUCommandBuffer* commandBuffer = nullptr);

		void Bind(SDL_GPURenderPass* renderPass, uint32_t slot) const;

		SDL_GPUBuffer* GetHandle();

	private:
		SDL_GPUBuffer* m_handle = nullptr;
		uint64_t m_size = 0;

		void Copy(const VertexBuffer& other);
		void Destroy();
	};
}
