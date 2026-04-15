#pragma once

#include "Angle.hpp"

#include "glm/mat4x4.hpp"

namespace fs {
	class Transform {
	public:
		Transform() = default;
		virtual ~Transform() = default;

		virtual void SetOrigin(glm::vec2 origin);

		virtual void Rotate(Angle angle);
		virtual void SetRotation(Angle angle);

		virtual void Move(glm::vec2 offset);
		virtual void SetPosition(glm::vec2 position);

		virtual void Scale(glm::vec2 scale);
		virtual void SetScale(glm::vec2 scale);

		glm::vec2 GetOrigin() const;
		glm::vec2 GetPosition() const;
		glm::vec2 GetScale() const;
		Angle GetRotation() const;

		glm::mat4 GetTransform() const;

	private:
		Angle m_rotation;

		glm::vec2 m_scale	 = glm::vec2(1.0f, 1.0f);

		glm::vec2 m_position = glm::vec2(0.0f, 0.0f);
		glm::vec2 m_origin   = glm::vec2(0.0f, 0.0f);
	};
}
