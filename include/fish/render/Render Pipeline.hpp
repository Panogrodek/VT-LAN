#pragma once

#include <cstdint>
#include <vector>

#include "graphics/Shader Moudle.hpp"
#include "graphics/Vertex Buffer.hpp"
#include "graphics/Texture.hpp"

#include "Shader Uniform Storage.hpp"

struct SDL_GPUGraphicsPipeline;
struct SDL_GPURenderPass;

namespace fs {
	enum class VertexInputRate : uint8_t {
		Vertex
	};

	enum class ShaderDataType : uint8_t {
		None = 0, Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Byte2, Byte4
	};

	enum class PrimitiveType : uint8_t {
		TriangleList,
		TriangleStrip
	};

	template<typename T>
	class ElementLayout { 
	public:
		ElementLayout() = default;
		virtual ~ElementLayout() = default;

		ElementLayout(std::initializer_list<T> elements) 
			: m_Elements(elements)
		{ }

		inline const std::vector<T>& GetElements() const { return m_Elements; }
		inline		 std::vector<T>& GetElements()		 { return m_Elements; }

		typename std::vector<T>::iterator begin() { return m_Elements.begin(); }
		typename std::vector<T>::iterator end()   { return m_Elements.end();   }

		typename std::vector<T>::const_iterator begin() const { return m_Elements.begin(); }
		typename std::vector<T>::const_iterator end()   const { return m_Elements.end();   }

	protected:
		std::vector<T> m_Elements;
	};

	struct VertexAttribute {
		VertexAttribute()  = default;
		~VertexAttribute() = default;

		VertexAttribute(ShaderDataType type);

		ShaderDataType Type;
		uint32_t	   Size;
		uint32_t	   Offset;
		uint32_t	   Location = 0;
	};

	class VertexAttributeLayout : public ElementLayout<VertexAttribute> {
	public:
		VertexAttributeLayout()  = default;
		~VertexAttributeLayout() = default;

		VertexAttributeLayout(std::initializer_list<VertexAttribute> elements);

		uint32_t Slot = 0;
	};

	class VertexBufferAttributeLayout : public ElementLayout<VertexAttributeLayout> {
	public:
		VertexBufferAttributeLayout()  = default;
		~VertexBufferAttributeLayout() = default;

		VertexBufferAttributeLayout(std::initializer_list<VertexAttributeLayout> elements);

		void SetSlot(uint32_t layoutIndex, uint32_t layoutSlot);
	};

	struct VertexBufferDescription {
		VertexInputRate InputRate;
		uint32_t		Slot = 0;
		uint32_t		VertexSize = 0;
		uint32_t		InstanceStepRate = 0;

		VertexBufferDescription()  = default;
		~VertexBufferDescription() = default;

		VertexBufferDescription(uint32_t vertexSize, uint32_t slot = 0, uint32_t instanceStepRate = 0, VertexInputRate inputRate = VertexInputRate::Vertex);
	};

	class VertexBufferDescriptions : public ElementLayout<VertexBufferDescription> {
	public:
		VertexBufferDescriptions()  = default;
		~VertexBufferDescriptions() = default;

		VertexBufferDescriptions(std::initializer_list<VertexBufferDescription> elements);
	};

	struct ColorTargetAttribute {
		TextureFormat Format = TextureFormat::RGBA_8B;
		// . . . blend state . . . //

		ColorTargetAttribute(TextureFormat format = TextureFormat::RGBA_8B);
	};

	class ColorTargetAttributeLayout : public ElementLayout<ColorTargetAttribute> {
	public:
		ColorTargetAttributeLayout()  = default;
		~ColorTargetAttributeLayout() = default;

		ColorTargetAttributeLayout(std::initializer_list<ColorTargetAttribute> elements);

		TextureFormat DepthStencilFormat  = TextureFormat::Depth_32F;
		bool		  EnableDepthStencil  = false;
		bool		  EnableAlphaBlending = false; //FIXME: this is temporary
	};

	struct PipelineData {
		ShaderModule* VertexShader	 = nullptr;
		ShaderModule* FragmentShader = nullptr;

		VertexBufferDescriptions    VertexBufferDescriptions;
		VertexBufferAttributeLayout VertexBuffersAttributeLayouts;
		ColorTargetAttributeLayout  ColorTargetsLayouts;

		PrimitiveType PrimitiveType = PrimitiveType::TriangleList;
	};

	class RenderPipeline : public ShaderUniformStorage {
	public:
		RenderPipeline() = default;
		~RenderPipeline();

		RenderPipeline(const RenderPipeline& other) = delete;
		RenderPipeline(const RenderPipeline&& other) = delete;

		RenderPipeline& operator=(const RenderPipeline& other) = delete;
		RenderPipeline& operator=(const RenderPipeline&& other) = delete;

		void Create(const PipelineData& pipelineData);
		void Bind(SDL_GPURenderPass* renderPass);

		SDL_GPUGraphicsPipeline* GetHandle();

		const PipelineData& GetPipelineData() const;

	private:
		PipelineData m_data;

		SDL_GPUGraphicsPipeline* m_handle = nullptr;

		void Destroy();

		static uint32_t GetVertexInputRate(VertexInputRate inputRate);
		static uint32_t GetPrimitiveType(PrimitiveType primitiveType);
		static uint32_t GetDataType(ShaderDataType dataType);
	};

	uint32_t GetShaderDataTypeSize(ShaderDataType type);
}
