#pragma once

#include "Texture Rect.hpp"

#include "utilities/Transform.hpp"
#include "utilities/Bounding Box.hpp"

#include "render/Layer.hpp"

namespace fs {
	class BaseShape : public Transform {
	public:
		BaseShape() = default;
		virtual ~BaseShape() = default;

		virtual void SetOrigin(glm::vec2 origin) override;

		virtual void Rotate(Angle angle) override;
		virtual void SetRotation(Angle angle) override;

		virtual void Move(glm::vec2 offset) override;
		virtual void SetPosition(glm::vec2 position) override;

		virtual void Scale(glm::vec2 scale) override;
		virtual void SetScale(glm::vec2 scale) override;

		void SetLayer(Layer layer);

		glm::mat4 GetTransform() const;

		const BoundingBox& GetAABB() const;
		const Layer GetLayer() const;

	protected:
		mutable bool m_recalculateAABB = false; // if true aabb needs recalculation
		mutable BoundingBox m_aabb;

		Layer m_layer = Layers::Default;

		virtual void RecalculateAABB() const = 0;
	};
}
