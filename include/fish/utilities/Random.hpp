#pragma once

#include <random>

#include "core/Runtime.hpp"

namespace fs {
	class Random {
	public:
		template<typename T>
		static T Rand(T min, T max) {
			return std::uniform_real_distribution<T>(min, max)(s_randEngine);
		}

		static int Rand(int min, int max) {
			return std::uniform_int_distribution<int>(min, max)(s_randEngine);
		}

		static uint32_t Rand(uint32_t min, uint32_t max) {
			return std::uniform_int_distribution<uint32_t>(min, max)(s_randEngine);
		}

	private:
		Random()  = default;
		~Random() = default;

		static void Initialize();

		static uint32_t GetSeed();

		static std::mt19937 s_randEngine;

		friend bool Runtime::Initialize();
	};

	class SeededRandom {
	public:
		SeededRandom();
		~SeededRandom() = default;

		void SetSeed(uint32_t seed);
		uint32_t GetSeed() const;

		template<typename T>
		T Rand(T min, T max) {
			return std::uniform_real_distribution<T>(min, max)(m_randEngine);
		}

		int Rand(int min, int max) {
			return std::uniform_int_distribution<int>(min, max)(m_randEngine);
		}

		uint32_t Rand(uint32_t min, uint32_t max) {
			return std::uniform_int_distribution<uint32_t>(min, max)(m_randEngine);
		}

	private:
		std::mt19937 m_randEngine;
		uint32_t m_seed = 0;
	};
}
