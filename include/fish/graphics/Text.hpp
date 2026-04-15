#pragma once

#include <vector>
#include <string>

#include "Font.hpp"
#include "Vertex.hpp"

#include "render/Layer.hpp"

#include "graphics/Texture.hpp"

#include "utilities/Transform.hpp"
#include "utilities/Bounding Box.hpp"

struct TTF_Text;
struct SDL_GPUTexture;

namespace fs {
	struct TextSequence {
		SDL_GPUTexture* Texture = nullptr;
		uint32_t VertexCount = 0;
		uint32_t IndexCount  = 0;
	};

	class Text : public Transform {
	public:
		Text() = default;
		~Text();

		Text(const Text& other) = delete;
		Text(const Text&& other) = delete;

		Text& operator=(const Text& other) = delete;
		Text& operator=(const Text&& other) = delete;

		void SetFont(Font& font);
		void SetString(const std::string& text);

		void SetLayer(Layer layer);

		void SetSmooth(bool smooth);

		void SetColor(glm::vec4 color);

		glm::vec2 GetSize() const;

		glm::vec4 GetColor() const;

		const std::string& GetString() const;

		Layer GetLayer() const;

		const std::vector<Vertex>& GetVertices()  const;
		const std::vector<uint32_t>& GetIndices() const;
		const std::vector<TextSequence>& GetSequences() const;

		const BoundingBox& GetAABB() const;

		SamplerType GetSamplerType() const;

	private:
		TTF_Text* m_handle = nullptr;
		Font* m_font = nullptr;

		glm::vec4 m_color = glm::vec4(1.0f);

		bool m_smooth = true;

		mutable bool m_updateDrawSequence = false;
		mutable bool m_updateVertices	  = false;

		std::string m_string = "";

		Layer m_layer = Layers::Default;

		mutable BoundingBox m_aabb;

		mutable std::vector<Vertex>   m_Vertices;
		mutable std::vector<uint32_t> m_Indices;
		mutable std::vector<TextSequence> m_Sequences;

		void BuildRenderSequence() const;

		void Create();
		void Destroy();
	};
}
