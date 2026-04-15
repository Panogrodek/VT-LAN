#pragma once

#include <cstdint>

struct SDL_GPUBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUCommandBuffer;

namespace fs {
	enum class ShaderStorageMode : uint32_t {
		Invalid      = 0,
		ShaderRead   = 1 << 0,
		ComputeRead  = 1 << 1,
		ComputeWrite = 1 << 2
	};

	inline ShaderStorageMode operator|(ShaderStorageMode a, ShaderStorageMode b) {
		return (ShaderStorageMode)(uint32_t(a) | uint32_t(b));
	}

	inline ShaderStorageMode& operator|=(ShaderStorageMode& a, ShaderStorageMode b) {
		a = ShaderStorageMode((uint32_t)a | (uint32_t)b);
		return a;
	}

	inline ShaderStorageMode operator&(ShaderStorageMode a, ShaderStorageMode b) {
		return ShaderStorageMode(uint32_t(a) & uint32_t(b));
	}

	class ShaderStorageBuffer {
	public:
		ShaderStorageBuffer() = default;
		~ShaderStorageBuffer();

		ShaderStorageBuffer(const ShaderStorageBuffer& other);
		ShaderStorageBuffer(const ShaderStorageBuffer&& other) noexcept;

		ShaderStorageBuffer& operator=(const ShaderStorageBuffer& other);
		ShaderStorageBuffer& operator=(const ShaderStorageBuffer&& other) noexcept;

		void Create(uint64_t size, ShaderStorageMode mode);
		void Create(void* data, uint64_t size, ShaderStorageMode mode);

		void Update(void* data, uint64_t size, SDL_GPUCommandBuffer* commandBuffer = nullptr);

		void BindToVertex(SDL_GPURenderPass* renderPass, uint32_t slot) const;
		void BindToFragment(SDL_GPURenderPass* renderPass, uint32_t slot) const;

		SDL_GPUBuffer* GetHandle();

		bool ContainsMode(ShaderStorageMode mode) const;

	private:
		SDL_GPUBuffer* m_handle = nullptr;

		ShaderStorageMode m_mode = ShaderStorageMode::Invalid;

		uint64_t m_size = 0;

		static uint32_t GetSDLUsage(ShaderStorageMode mode);

		void Copy(const ShaderStorageBuffer& other);
		void Destroy();
	};
}
