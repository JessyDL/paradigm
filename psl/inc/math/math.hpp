#pragma once
#include "vec.h"
#include "matrix.h"
#include "quaternion.h"
#include "utility.h"

#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

namespace psl
{

	template <typename precision_t>
	constexpr psl::tvec<precision_t, 3> operator*(const psl::tquat<precision_t>& quat, const psl::tvec<precision_t, 3>& vec) noexcept
	{
		const tvec<precision_t, 3> qVec{ quat[0], quat[1], quat[2]};
		const tvec<precision_t, 3> uv(cross(qVec, vec));
		const tvec<precision_t, 3> uuv(cross(qVec, uv));

		return vec + ((uv * quat[3]) + uuv) * precision_t { 2 };
	}
}

namespace psl::math
{
	template <typename precision_t>
	constexpr static precision_t sin(precision_t value) noexcept
	{
		return std::sin(value);
	}
	template <typename precision_t>
	constexpr static precision_t cos(precision_t value) noexcept
	{
		return std::cos(value);
	}
	template<typename precision_t>
	constexpr static precision_t tan(precision_t value) noexcept
	{
		return std::tan(value);
	}
	template <typename precision_t>
	constexpr static precision_t acos(precision_t value) noexcept
	{
		return std::acos(value);
	}
	template <typename precision_t>
	constexpr static precision_t asin(precision_t value) noexcept
	{
		return std::asin(value);
	}
	template<typename precision_t>
	constexpr static precision_t atan(precision_t value) noexcept
	{
		return std::atan(value);
	}

	template <typename precision_t>
	constexpr static tquat<precision_t> angle_axis(const precision_t& angle,
												   const psl::tvec<precision_t, 3>& vec) noexcept
	{
		const precision_t s = sin(angle * precision_t{0.5});
		return tquat<precision_t> { vec[0] * s, vec[1] * s, vec[2] * s, cos(angle * precision_t {0.5})};
	}

	template <typename precision_t>
	constexpr static psl::tvec<precision_t, 3> rotate(const psl::tquat<precision_t>& quat, const psl::tvec<precision_t, 3>& vec) noexcept
	{
		return quat * vec;
	}
} // namespace psl::math