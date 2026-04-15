#pragma once

#include <unordered_map>

#include "Texture.hpp"
#include "utilities/Uber Texture Builder.hpp"
#include "utilities/Resource Loader.hpp"

namespace fs {
	class UberTexture;

	struct UberTextureChunk {
		UberTextureRect Rect;
		UberTexture*	UberTexture = nullptr;
	};

	class UberTexture {
	public:
		UberTexture()  = default;
		~UberTexture() = default;

		void Append(const std::string& name, Texture& texture);
		void Load(const std::string& name, const std::string& path, fs::TextureFilteringMode filteringMode = fs::TextureFilteringMode::Linear);

		void Build();

		Texture& GetTexture();
		const Texture& GetTexture() const;

		UberTextureChunk GetChunk(const std::string& name);
		UberTextureChunk operator[](const std::string& name);

	private:
		Texture m_texture;

		fs_priv::ResourceLoader m_loader;

		std::vector<Texture*> m_TexturesToBuild;
		std::vector<Texture*> m_LoadedTextures;

		std::vector<std::pair<Texture*, std::string>> m_TextureNames;

		std::unordered_map<Texture*, UberTextureRect> m_TempRects;
		std::unordered_map<std::string, UberTextureChunk> m_Chunks;
	};
}
