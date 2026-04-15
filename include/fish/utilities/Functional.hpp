#pragma once

#include <cstdint>
#include <bitset>
#include <cmath>

namespace fs {
	class UberTexture;

	static uint64_t HashTextures(UberTexture* a, UberTexture* b) {
		if (!a && !b) 
			return 0;

		std::hash<UberTexture*> hasher;
		std::size_t h1 = a ? hasher(a) : 0;
		std::size_t h2 = b ? hasher(b) : 0;

		std::size_t seed = h1;
		seed ^= h2 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
		return seed;
	}

	template<typename T, typename O>
	static bool Contains(const T& map, const O& obj) {
		return map.find(obj) != map.end();
	}

	static float EaseOutQuint(float x) {
		return 1.0f - std::pow(1.0f - x, 5.0f);
	}

	static float EaseInQuint(float x) {
		return x * x * x * x * x;
	}

	// converts two dimensional indexes into 1D
	inline uint32_t In2D(uint32_t x, uint32_t y, uint32_t sizeX) {
		return x + y * sizeX;
	}
}
