#pragma once

#include "Base Shape.hpp"
#include "Texture.hpp"

namespace fs {
	class Quad : public BaseShape {
	public:
		Quad();
		~Quad() = default;

		void SetSize(glm::vec2 size, bool center = false);
		void SetColor(glm::vec4 color);

		void SetTexture(Texture* texture);
		void SetNormalTexture(Texture* texture);

		void SetTextureRect(TextureRect rect);

		glm::vec2 GetSize() const;
		glm::vec4 GetColor() const;

		const Vertex* GetVertices() const;

		Texture*	   GetTexture();
		const Texture* GetTexture() const;

		Texture*	   GetNormalTexture();
		const Texture* GetNormalTexture() const;

	private:
		Vertex m_Vertices[6];
		
		TextureRect m_textureRect;

		Texture* m_texture		 = nullptr;
		Texture* m_normalTexture = nullptr;

		glm::vec2 m_size  = glm::vec2(0.0f);
		glm::vec4 m_color = glm::vec4(1.0f);

		void UpdateTexCoords();
		virtual void RecalculateAABB() const override;
	};
}
