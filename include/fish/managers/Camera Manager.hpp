#pragma once

#include <string>

#include "core/Runtime.hpp"

#include "render/Camera.hpp"

namespace fs {
	namespace fs_priv {
		class CameraManager {
		public:
			CameraManager()  = default;
			~CameraManager() = default;

			Camera& CreateCamera(const std::string& name, glm::vec2 size, bool center = true);
			Camera& CreateCamera(const std::string& name, glm::vec2 size, glm::vec2 center);
			Camera& CreateCamera(const std::string& name, Camera& camera);

			Camera& GetCamera(const std::string& name);

			Camera& operator[](const std::string& name);

			void BindCamera(Camera& camera);
			void UnbindCamera(Camera& camera);

			std::unordered_map<std::string, Camera>& GetCameras();

			const Camera& GetCurrentCamera() const;
			Camera& GetDefault();

		private:
			std::unordered_map<std::string, Camera> m_Cameras;

			Camera m_defaultCamera;

			Camera* m_currentCamera = nullptr;

			friend bool Runtime::Initialize();
			void Initialize();
		};
	}

	inline fs_priv::CameraManager CameraManager;
}
