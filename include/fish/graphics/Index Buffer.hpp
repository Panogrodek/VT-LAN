#pragma once

#include <cstdint>

struct SDL_GPUBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUCommandBuffer;

namespace fs {
	class IndexBuffer {
	public:
		IndexBuffer() = default;
		~IndexBuffer();

		IndexBuffer(const IndexBuffer& other);
		IndexBuffer(const IndexBuffer&& other) noexcept;

		IndexBuffer& operator=(const IndexBuffer& other);
		IndexBuffer& operator=(const IndexBuffer&& other) noexcept;

		void Create(uint64_t size);
		void Create(void* data, uint64_t size);

		void Update(void* data, uint64_t size, SDL_GPUCommandBuffer* commandBuffer = nullptr);

		void Bind(SDL_GPURenderPass* renderPass) const;

		SDL_GPUBuffer* GetHandle();

	private:
		SDL_GPUBuffer* m_handle = nullptr;
		uint64_t m_size = 0;

		void Copy(const IndexBuffer& other);
		void Destroy();
	};
}
