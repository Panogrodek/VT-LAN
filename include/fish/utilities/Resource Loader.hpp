#pragma once

#include <functional>
#include <vector>
#include <mutex>
#include <future>

namespace fs {
	class Texture;

	enum class ResourceType {
		Texture,
	};

	namespace fs_priv {
		class ResourceLoader {
		public:
			ResourceLoader()  = default;
			~ResourceLoader() = default;

			void Sync();

			void Load(const std::string& name, const std::string& path, ResourceType type);
			void Load(Texture* resource, const std::string& path);

		private:
			struct ReturnVal {
				bool Good = false;
				ResourceType Type;
				std::string  Name;
				void* Data = nullptr;
			};

			struct Resource {
				std::function<ReturnVal(void*, std::string, std::string)> Load = 0;
				void* Data = nullptr;
			};

			std::vector<std::future<ReturnVal>> m_Results;

			Resource GetResource(const std::string& name, ResourceType type, bool buildEmptyData = true);

			void BuildResource(ReturnVal value);
			void BuildTexture(const std::string& name, Texture* texture, bool good);

			static ReturnVal LoadTexture(void* res, std::string name, std::string path);
		};
	}

	inline fs_priv::ResourceLoader ResourceLoader;
}
