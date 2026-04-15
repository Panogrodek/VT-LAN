#pragma once

#include <unordered_map>

#include "Input Layer.hpp"

namespace fs {
	namespace fs_priv {
		class Input {
		public:
			Input()  = default;
			~Input() = default;

			void Update();

			void AddLayer(const std::string& name);

			InputLayer& Get(const std::string& name);

			InputLayer& operator[](const std::string& name);

			std::unordered_map<std::string, InputLayer>& GetLayers();

		private:
			std::unordered_map<std::string, InputLayer> m_Layers;
		};
	}

	inline fs_priv::Input Input;
}
