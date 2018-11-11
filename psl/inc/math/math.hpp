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
	constexpr psl::tvec<precision_t, 3> operator*(const psl::tquat<precision_t>& quat,
												  const psl::tvec<precision_t, 3>& vec) noexcept
	{
		const tvec<precision_t, 3> qVec{quat[0], quat[1], quat[2]};
		const tvec<precision_t, 3> uv(psl::math::cross(qVec, vec));
		const tvec<precision_t, 3> uuv(psl::math::cross(qVec, uv));

		return vec + ((uv * quat[3]) + uuv) * precision_t{2};
	}
} // namespace psl

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
	template <typename precision_t>
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
	template <typename precision_t>
	constexpr static precision_t atan(precision_t value) noexcept
	{
		return std::atan(value);
	}

	template <typename precision_t>
	constexpr static tquat<precision_t> angle_axis(const precision_t& angle,
												   const psl::tvec<precision_t, 3>& vec) noexcept
	{
		const precision_t s = sin(angle * precision_t{0.5});
		return tquat<precision_t>{vec[0] * s, vec[1] * s, vec[2] * s, cos(angle * precision_t{0.5})};
	}

	template <typename precision_t>
	constexpr static psl::tvec<precision_t, 3> rotate(const psl::tquat<precision_t>& quat,
													  const psl::tvec<precision_t, 3>& vec) noexcept
	{
		return quat * vec;
	}
} // namespace psl::math


/// conversions
namespace psl::math
{

	template <typename precision_t>
	constexpr static tquat<precision_t> from_euler(precision_t pitch, precision_t yaw, precision_t roll) noexcept
	{
		constexpr precision_t half{precision_t{1} / precision_t{2}};
		pitch		   = psl::math::radians(pitch) * half;
		yaw			   = psl::math::radians(yaw) * half;
		roll		   = psl::math::radians(roll) * half;
		precision_t cy = cos(roll);
		precision_t sy = sin(roll);
		precision_t cr = cos(yaw);
		precision_t sr = sin(yaw);
		precision_t cp = cos(pitch);
		precision_t sp = sin(pitch);

		return tquat<precision_t>{cy * cr * cp + sy * sr * sp, cy * cr * sp + sy * sr * cp, cy * sr * cp - sy * cr * sp,
								  sy * cr * cp - cy * sr * sp};
	}
	template <typename precision_t>
	constexpr static tquat<precision_t> from_euler(const psl::tvec<precision_t, 3>& vec) noexcept
	{
		return from_euler(vec[0], vec[1], vec[2]);
	}

	template <typename precision_t>
	constexpr static tvec<precision_t, 3> to_euler(const tquat<precision_t>& q) noexcept
	{
		tvec<precision_t, 3> res;
		precision_t sqw  = q[3] * q[3];
		precision_t sqx  = q[0] * q[0];
		precision_t sqy  = q[1] * q[1];
		precision_t sqz  = q[2] * q[2];
		precision_t unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
		precision_t test = q[0] * q[1] + q[2] * q[3];
		if(test > precision_t{100} / precision_t{201} * unit)
		{ // singularity at north pole
			res[0] = precision_t{2} * atan2(q[0], q[3]);
			res[1] = PI / precision_t{2};
			res[2] = precision_t{0};
		}
		else if(test < -precision_t{100} / precision_t{201} * unit)
		{ // singularity at south pole
			res[0] = -precision_t{2} * atan2(q[0], q[3]);
			res[1] = -PI / precision_t{2};
			res[2] = 0;
		}
		else
		{
			res[0] = atan2(precision_t{2} * q[1] * q[3] - 2 * q[0] * q[2], sqx - sqy - sqz + sqw);
			res[1] = asin(precision_t{2} * test / unit);
			res[2] = atan2(precision_t{2} * q[0] * q[3] - 2 * q[1] * q[2], -sqx + sqy - sqz + sqw);
		}
		res[0] = degrees(res[0]);
		res[1] = degrees(res[1]);
		res[2] = degrees(res[2]);
		return res;
	}


	template <typename precision_t>
	constexpr static psl::tmat<precision_t, 4, 4> perspective_projection(precision_t vertical_fov,
																		 precision_t aspect_ratio, precision_t near,
																		 precision_t far) noexcept
	{
		precision_t const thf = tan(vertical_fov / precision_t{2});
		psl::tmat<precision_t, 4, 4> res(precision_t{0});
		res[{0, 0}] = precision_t{1} / (aspect_ratio * thf);
		res[{1, 1}] = precision_t{1} / (thf);
		res[{2, 3}] = -precision_t{1};
		res[{2, 2}] = far / (near - far);
		res[{3, 2}] = -(far * near) / (far - near);
		return res;
	}

	template <typename precision_t>
	constexpr static psl::tmat<precision_t, 4, 4> look_at(const psl::tvec<precision_t, 3>& eye,
														  const psl::tvec<precision_t, 3>& center,
														  const psl::tvec<precision_t, 3>& up) noexcept
	{
		psl::tvec<precision_t, 3> const f(normalize(center - eye));
		psl::tvec<precision_t, 3> const s(normalize(cross(f, up)));
		psl::tvec<precision_t, 3> const u(cross(s, f));

		psl::tmat<precision_t, 4, 4> res{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
		res[{0, 0}] = s[0];
		res[{1, 0}] = s[1];
		res[{2, 0}] = s[2];
		res[{0, 1}] = u[0];
		res[{1, 1}] = u[1];
		res[{2, 1}] = u[2];
		res[{0, 2}] = -f[0];
		res[{1, 2}] = -f[1];
		res[{2, 2}] = -f[2];
		res[{3, 0}] = -dot(s, eye);
		res[{3, 1}] = -dot(u, eye);
		res[{3, 2}] = dot(f, eye);
		return res;
	}
} // namespace psl::math