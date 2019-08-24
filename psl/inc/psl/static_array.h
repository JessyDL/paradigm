#pragma once
#include <array>

namespace psl
{
	template<typename T, size_t N = 0>
	using static_array = std::array<T, N>;


	template<typename T>
	class range
	{
	public:
		static_assert(std::is_integral_v<T>, "type must be integral type");
		range() = default;
		range(const std::pair<T, T>& values) : m_First(values.first), m_Last(values.second) {};
		range(const T& first, const T& last) : m_First(first), m_Last(last) {};
		T size() const noexcept { return m_Last - m_First; };
		T begin() { return m_First; }
		T end() { return m_Last; }

		void begin(T value) { m_First = value; }
		void end(T value) { m_Last = value; }
	private:
		T m_First{};
		T m_Last{};
	};
}