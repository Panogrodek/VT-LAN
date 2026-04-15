#pragma once

#include "Base Shape.hpp"
#include "Uber Texture.hpp"

namespace fs {
	class BatchQuad : public BaseShape {
	public:
		BatchQuad();
		~BatchQuad() = default;

		void SetSize(glm::vec2 size, bool center = false);
		void SetColor(glm::vec4 color);

		void SetTexture(UberTextureChunk uberTexture);
		void SetNormalTexture(UberTextureChunk uberTexture);

		glm::vec2 GetSize() const;
		glm::vec4 GetColor() const;

		const BatchVertex* GetVertices() const;

		UberTextureChunk GetTexture();
		UberTextureChunk GetTexture() const;

		UberTextureChunk GetNormalTexture();
		UberTextureChunk GetNormalTexture() const;

	private:
		BatchVertex m_Vertices[6];

		UberTextureChunk m_texture;
		UberTextureChunk m_normalTexture;

		glm::vec2 m_size  = glm::vec2(0.0f);
		glm::vec4 m_color = glm::vec4(1.0f);

		void UpdateTexCoords();
		void UpdateNormalTexCoords();

		virtual void RecalculateAABB() const override;
	};
}
