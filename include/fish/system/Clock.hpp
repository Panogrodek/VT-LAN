#pragma once

#include <chrono>

namespace fs {
	class Clock {
	public:
		Clock();
		~Clock() = default;

		void Start();
		void Stop();

		void Reset();
		double Restart();

		double GetElapsedTimeAsSeconds() const;

	private:
		std::chrono::time_point<std::chrono::steady_clock> m_startTime;
		std::chrono::duration<double> m_elapsed;
		bool m_running;
	};
}
