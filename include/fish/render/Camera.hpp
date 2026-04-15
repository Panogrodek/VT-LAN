#pragma once

#include "utilities/Bounding Box.hpp"

#include "glm/vec2.hpp"
#include "glm/mat4x4.hpp"

namespace fs {
	namespace fs_priv {
		class CameraManager;
	}

	class Camera {
	public:
		Camera()  = default;
		~Camera() = default;

		void Create(glm::vec2 size, bool center = true);
		void Create(glm::vec2 size, glm::vec2 center);

		void Bind();
		void Unbind();

		void Move(glm::vec2 offset);

		void SetCenter(glm::vec2 center);
		void SetScale(float scale);

		void SetAvailableLight(bool available);

		float GetScale() const;

		glm::vec2 GetCenter() const;
		glm::vec2 GetSize() const;

		glm::vec2 Camera::MapScreenToWorld(glm::vec2 position) const;
		glm::vec2 Camera::MapScreenToWorld(glm::vec2 position, glm::vec2 viewportSize) const;

		bool InFrustum(glm::vec2 point, glm::mat4 transform = glm::mat4(1.0f)) const;
		bool InFrustum(const BoundingBox& boundingBox, glm::mat4 transform = glm::mat4(1.0f)) const;
		bool InFrustum(const glm::vec2* Points, uint32_t count, glm::mat4 transform = glm::mat4(1.0f)) const;

		bool IsLightAvailable() const;

		glm::mat4 GetView() const;
		glm::mat4 GetProjection() const;

	private:
		Camera* m_previousCamera = nullptr;

		glm::vec2 m_size   = glm::vec2(1.0f);
		glm::vec2 m_center = glm::vec2(1.0f);
		float	  m_scale  = 1.0f;

		glm::mat4 m_projection = glm::mat4(1.0f);

		bool m_lightAvailable = true;

		void UpdateProjection();

		friend class fs_priv::CameraManager;
	};
}
