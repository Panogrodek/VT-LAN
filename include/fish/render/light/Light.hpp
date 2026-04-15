#pragma once

#include "render/Layer.hpp"
#include "Light Data.hpp"

#include "glm/vec2.hpp"

namespace fs {
	class Light {
	public:
		Light(LightType type);
		virtual ~Light();

		Light(const Light& other);
		Light(const Light&& other) = delete;

		Light& operator=(const Light& other);
		Light& operator=(const Light&& other) = delete;

		void SetPosition(glm::vec2 position);
		void SetColor(glm::vec3 color);
		void Move(glm::vec2 offset);

		void SetActive(bool active);
		void SetFallthrough(bool fallthrough);

		void SetLayer(Layer layer);

		glm::vec2 GetPosition() const;
		glm::vec3 GetColor() const;

		bool IsActive() const;

		LightData GetDataCopy() const;
		LightType GetType() const;

	protected:
		LightData m_data;

	private:
		LightType m_type = LightType::Invalid;

		bool m_active	   = true;
		bool m_fallthrough = true;

		void Copy(const Light& other);
	};
}
