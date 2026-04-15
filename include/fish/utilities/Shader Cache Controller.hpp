#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace fs {
	namespace fs_priv {
		class ShaderCacheController {
		public:
			ShaderCacheController()  = default;
			~ShaderCacheController() = default;

			bool CheckCachePath();
			void CreateCachePath();

			void SetCachePath(const std::string& path);

			const std::string& GetPath() const;

			void LoadBuildInfo();
			void CacheShader(void* data, uint64_t size, const std::string& path, bool checkModificationTime = true);

			void AddCacheEntry(const std::string& path, bool checkModificationTime = true);

			bool FindCachedFile(const std::string& shader);
			bool CheckForEdition(const std::string& path);

		private:
			uint32_t m_shaderCount = 1;

			std::string m_cachePath = "res/shader_build";

			struct ShaderVersion {
				uint32_t NameSize	   = 0;
				int64_t  EditTimestemp = 0;
				uint8_t  CompileMode   = 0; // mode in which the shader was compiled, debug or release (unoptimized or optimized), 0 is debug 1 is release
			};

			std::unordered_map<std::string, ShaderVersion> m_ShaderCacheInfo;
		};
	}

	inline fs_priv::ShaderCacheController ShaderCacheController;
}
