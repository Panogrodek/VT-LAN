#pragma once

#include "glm/vec2.hpp"
#include <string>

struct SDL_GPUTexture;
struct SDL_GPURenderPass;
struct SDL_Surface;

namespace fs {
	enum class TextureFormat {
		RGBA_8B,
		RGBA_16F,
		RGBA_32F,
		Depth_32F
	};

	enum class TextureChannels {
		R,
		RG,
		RGB,
		RGBA,
	};

	enum class TextureUsage : uint32_t {
		Invalid						 = 0,
		Sampler						 = 1 << 0,
		ColorTraget					 = 1 << 1,
		DepthSetncilTarget			 = 1 << 2,
		ShaderRead					 = 1 << 3,
		ComputeRead					 = 1 << 4,
		ComputeWrite				 = 1 << 5,
		ComputeSimultaneousReadWrite = 1 << 6
	};

	inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
		return TextureUsage((uint32_t)a | (uint32_t)b);
	}

	inline TextureUsage& operator|=(TextureUsage& a, TextureUsage b) {
		a = TextureUsage((uint32_t)a | (uint32_t)b);
		return a;
	}

	inline TextureUsage operator&(TextureUsage a, TextureUsage b) {
		return TextureUsage((uint32_t)a & (uint32_t)b);
	}

	enum class SamplerType : uint32_t { // Clamp -> clamp texture, Wrap -> repeat texture
		PointClamp = 0,
		PointWrap,
		LinearClamp,
		LinearWrap,
		AnisotropicClamp,
		AnisotropicWrap
	};

	enum class TextureFilteringMode {
		Linear,
		Nearest,
		Anisotropic
	};

	struct TextureData {
		TextureFormat		 Format        = TextureFormat::RGBA_8B;
		TextureUsage		 Usage         = TextureUsage::Sampler;
		TextureChannels		 Channels      = TextureChannels::RGBA;
		TextureFilteringMode FilteringMode = TextureFilteringMode::Linear;
		bool				 Repeated	   = true;
		glm::vec2		     Size		   = glm::vec2(1, 1);
	};

	class Texture {
	public:
		Texture()  = default;
		~Texture();
		
		Texture(const Texture& other);
		Texture(const Texture&& other) noexcept;

		Texture& operator=(const Texture& other);
		Texture& operator=(const Texture&& other) noexcept;

		bool LoadFromFile(const std::string& path);
		bool LoadAndHoldPixelData(const std::string& path);

		void Bind(SDL_GPURenderPass* renderPass, uint32_t slot) const;

		void Create(TextureData data);
		void Create(TextureData data, void* pixelData);

		void CreateFromPixelData();

		void Update(void* pixelData, uint64_t size);

		void SetFiltering(TextureFilteringMode filteringMode);
		void SetRepeated(bool repeated);

		glm::vec2 GetSize() const;

		bool ContainsUsage(TextureUsage usage) const;

		SamplerType GetSamplerType() const;
		TextureUsage GetUsage() const;

		SDL_GPUTexture* GetHandle();
		SDL_GPUTexture* GetHandle() const;

		static uint32_t GetSDLTextureFormat(TextureFormat format);
		static uint32_t GetSDLTextureUsage(TextureUsage usage);
		static uint32_t GetTextureChannelCount(TextureChannels channles);

	private:
		SDL_GPUTexture* m_handle = nullptr;

		SDL_Surface* m_tempSurface = nullptr;

		TextureData m_data;

		void Copy(const Texture& other);
		void Destroy();
	};
}
