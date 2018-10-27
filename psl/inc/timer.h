#pragma once
#include <chrono>

namespace psl
{
	class timer
	{
	public:
		timer() :
			m_Point(std::chrono::high_resolution_clock::now())
		{}
		void reset()
		{
			m_Point = std::chrono::high_resolution_clock::now();
		}

		uint64_t elapsed() const
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(
				std::chrono::high_resolution_clock::now() - m_Point).count();
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Point;
	};
}
