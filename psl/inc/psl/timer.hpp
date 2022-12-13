#pragma once
#include <chrono>

namespace psl
{
class timer
{
  public:
	timer() : m_Point(std::chrono::high_resolution_clock::now()) {}
	void reset() { m_Point = std::chrono::high_resolution_clock::now(); }

	template <typename type = std::chrono::nanoseconds>
	type elapsed() const
	{
		return std::chrono::duration_cast<type>(std::chrono::high_resolution_clock::now() - m_Point);
	}

  private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Point;
};
}	 // namespace psl
