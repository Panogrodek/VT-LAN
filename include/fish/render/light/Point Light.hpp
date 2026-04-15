#pragma once

#include "Light.hpp"

namespace fs {
	class PointLight : public Light {
	public:
		PointLight();
		~PointLight() = default;
	};
}
