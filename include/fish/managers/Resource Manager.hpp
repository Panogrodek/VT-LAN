#pragma once

#include <unordered_map>
#include <string>

#include "utilities/Functional.hpp"

#include "graphics/Texture.hpp"
#include "graphics/Uber Texture.hpp"
#include "graphics/Font.hpp"
#include "graphics/Shader.hpp"

#include "audio/AudioSource.hpp"

namespace fs {
	template<typename T>
	class ResourceManager {
	private:
		typedef std::unordered_map<std::string, T> Resources;

	public:
		ResourceManager() = default;
		virtual ~ResourceManager() = default;

		Resources& Raw() {
			return m_Resources;
		}

		virtual T& Get(const std::string& name) {
			DB_ONLY
			(
			if (!Contains(m_Resources, name))
				WLOG("Accessing not existing resource on name \"{}\" returning empty", name.c_str());
			)

			return m_Resources[name];
		}

		bool Load(std::string name, const std::string& path) {
			DB_ONLY
			(
			if (Contains(m_Resources, name)) {
				WLOG("Tried to override resource on name \"{}\" skipping", name.c_str());
				return false;
			}
			)

			bool good = m_Resources[name].LoadFromFile(path);

			if (!good)
				m_Resources.erase(name);

			return good;
		}

		void Unload(const std::string& name) {
			DB_ASSERT(Contains(m_Resources, name)); // Tried deleting resource that dosen't exist

			m_Resources.erase(name);
		}

		void Clear() {
			m_Resources.clear();
		}

		size_t Size() const {
			return m_Resources.size();
		}

		virtual T& operator[](const std::string& name) {
			return Get(name);
		}

	protected:
		Resources m_Resources;
	};

	namespace fs_priv {
		class FontManager {
		private:
			class FontSubClass {
			public:
				FontSubClass()  = default;
				~FontSubClass() = default;

				Font& Get(uint32_t size, bool warn = true) {
					DB_ONLY
					(
					if (!Contains(m_Fonts, size) && warn)
						WLOG("Accessing not existing font on size \"{}\" returning empty", size);
					)

					return m_Fonts[size];
				}

				Font& operator[](uint32_t size) {
					return Get(size);
				}

				void Erase(uint32_t size) {
					m_Fonts.erase(size);
				}

				std::unordered_map<uint32_t, Font>& Raw() {
					return m_Fonts;
				}

			private:
				std::unordered_map<uint32_t, Font> m_Fonts;
			};

		public:
			bool Load(const std::string& name, uint32_t size, const std::string& path) {
				DB_ONLY
				(
				if (Contains(m_Fonts[name].Raw(), size)) {
					WLOG("Tried to override resource on name \"{}\" skipping", name.c_str());
					return false;
				}
				)

				bool good = m_Fonts[name].Get(size, false).LoadFromFile(path, size);

				if (!good)
					m_Fonts[name].Erase(size);

				return good;
			}

			void Unload(const std::string& name, uint32_t size) {
				DB_ASSERT(Contains(m_Fonts[name].Raw(), size)); // Tried deleting resource that dosen't exist

				m_Fonts[name].Erase(size);
			}

			void Clear() {
				m_Fonts.clear();
			}

			virtual FontSubClass& Get(const std::string& name) {
				DB_ONLY
				(
				if (!Contains(m_Fonts, name))
					WLOG("Accessing not existing resource on name \"{}\" returning empty", name.c_str());
				)

				return m_Fonts[name];
			}

			virtual FontSubClass& operator[](const std::string& name) {
				return Get(name);
			}

		private:
			std::unordered_map<std::string, FontSubClass> m_Fonts;
		};

		class ShaderManager : public ResourceManager<Shader> {
		public:
			ShaderManager()  = default;
			~ShaderManager() = default;

			bool Load(std::string name, const std::string& path) = delete;

			bool Load(std::string name, const std::string& vertPath, const std::string& fragPath) {
				DB_ONLY
				(
				if (Contains(m_Resources, name)) {
					WLOG("Tried to override resource on name \"{}\" skipping", name.c_str());
					return false;
				}
				)

				bool good = m_Resources[name].LoadFromFile(vertPath, fragPath);

				if (!good)
					m_Resources.erase(name);

				return good;
			}
		};

		class UberTextureManager : public ResourceManager<UberTexture> {
		public:
			UberTextureManager()  = default;
			~UberTextureManager() = default;

			bool Load(std::string name, const std::string& path) = delete;

			void Load(std::string name, const std::string& chunkName, const std::string& path, fs::TextureFilteringMode filteringMode = fs::TextureFilteringMode::Linear) {
				m_Resources[name].Load(chunkName, path, filteringMode);
			}

			void BuildAll() {
				for (auto& [name, uberTexture] : m_Resources)
					uberTexture.Build();
			}
		};

		class AudioSourceManager : public ResourceManager<AudioSource> {
		public:
			AudioSourceManager() = default;
			~AudioSourceManager() = default;

			bool Load(std::string name, const std::string& path) = delete;

			bool Load(std::string name, const std::string& path, bool isMusic, MIX_Mixer* prefferedMixer = nullptr) {
				return m_Resources[name].LoadFromFile(path, isMusic, prefferedMixer);
			}
		};
	}

	inline ResourceManager<Texture>		TextureManager;
	inline fs_priv::FontManager			FontManager;
	inline fs_priv::ShaderManager		ShaderManager;
	inline fs_priv::UberTextureManager	UberTextureManager;
	inline fs_priv::AudioSourceManager	AudioSourceManager;
}
