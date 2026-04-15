#pragma once

#include "graphics/Vertex.hpp"

#include "glm/mat4x4.hpp"

namespace fs {
	class BoundingBox {
	public:
		BoundingBox()  = default;
		~BoundingBox() = default;

		bool Contains(glm::vec2 point) const;

		void Calculate(const Vertex* Vertices, uint32_t count, glm::mat4 transform);
		void Calculate(const BatchVertex* Vertices, uint32_t count, glm::mat4 transform);

		glm::vec2 GetPoint(uint32_t index) const;
		glm::vec2 GetMin() const;
		glm::vec2 GetMax() const;
		const glm::vec2* GetVertices() const;

	private:
		glm::vec2 m_Vertices[4];

		glm::vec2 m_min = glm::vec2(0.0f);
		glm::vec2 m_max = glm::vec2(0.0f);

		void CalculateVertices();
	};
}
