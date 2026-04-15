#pragma once

#include <functional>

#define DEFINE_LAYER(name) \
	inline const fs::Layer name

#define DEFINE_LAYER_NO_LIGHT(name) \
	inline const fs::Layer name{-1, false}

namespace fs {
	class Layer {
	public:
		Layer(int32_t order = -1, bool usesLight = true);

		bool operator<(const Layer& other) const;
		bool operator==(const Layer& other) const;
		bool operator!=(const Layer& other) const;

		static uint32_t GetLayerCount();
		static const std::vector<const Layer*> GetLayers();

		uint32_t GetOrder() const;
		bool LightAvailable() const;

	private:
		static uint32_t s_currentOrder;

		uint32_t m_order = 0;
		bool	 m_lightAvailable = true;

		static std::vector<const Layer*>& GetLayersStorage();
	};
}

namespace Layers {
	inline const fs::Layer Default(0);
}

namespace std {
	template<>
	struct hash<fs::Layer> {
		size_t operator()(const fs::Layer& layer) const noexcept {
			return std::hash<uint32_t>{}(layer.GetOrder());
		}
	};
}
