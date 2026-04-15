#pragma once

#include <string>
#include <vector>

struct SDL_GPUShader;

namespace fs {
	enum class ShaderType {
		Vertex,
		Fragment
	};

	class ShaderModule {
	public:
		ShaderModule() = default;
		~ShaderModule();

		ShaderModule(const ShaderModule& other)  = delete;
		ShaderModule(const ShaderModule&& other) = delete;

		ShaderModule& operator=(const ShaderModule& other)  = delete;
		ShaderModule& operator=(const ShaderModule&& other) = delete;

		bool LoadFromFile(const std::string& path, ShaderType type);
		bool LoadFromMemory(std::string shader, const std::string& cacheName, ShaderType type);

		SDL_GPUShader* GetHandle();

	private:
		SDL_GPUShader* m_handle = nullptr;

		ShaderType m_type = ShaderType::Vertex;

		struct ShaderObjectCount {
			uint32_t SamplerCount		= 0;
			uint32_t UniformBufferCount = 0;
			uint32_t StorageBufferCount = 0;
			uint32_t StorageImageCount  = 0;
		};

		bool CompileFromFile(const std::string& path);
		bool CompileFromMemory(std::string shader, const std::string& cacheName);

		bool BuildFromCacheFile(const std::string& path, bool checkModificationTime = true);

		void CreateShader(void* shaderSource, uint64_t size, ShaderObjectCount shaderObjectCount);

		ShaderObjectCount GetShaderObjectCount(void* source, uint64_t sourceSize);

		std::vector<uint32_t> CompileShader(void* source, uint64_t size, const std::string& fileName) const;
	};
}
