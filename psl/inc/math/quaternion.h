#pragma once

#define USE_SSE2

namespace psl
{
	template <typename precision_t>
	struct tquat
	{
		using tquat_t = tquat<precision_t>;

		const static tquat_t identity;

		constexpr tquat() noexcept = default;
		constexpr tquat(const precision_t& x, const precision_t& y, const precision_t& z, const precision_t& w) noexcept
			: value({x, y, z, w}){};
		constexpr tquat(precision_t&& x, precision_t&& y, precision_t&& z, precision_t&& w) noexcept
			: value({std::move(x), std::move(y), std::move(z), std::move(w)}){};
		constexpr tquat(const precision_t& value) noexcept : value({value, value, value, value}){};

		// ---------------------------------------------
		// getters
		// ---------------------------------------------
		constexpr precision_t& x() noexcept { return value[0]; }
		constexpr const precision_t& x() const noexcept { return value[0]; }
		constexpr precision_t& y() noexcept { return value[1]; }
		constexpr const precision_t& y() const noexcept { return value[1]; }
		constexpr precision_t& z() noexcept { return value[2]; }
		constexpr const precision_t& z() const noexcept { return value[2]; }
		constexpr precision_t& w() noexcept { return value[3]; }
		constexpr const precision_t& w() const noexcept { return value[3]; }

		// ---------------------------------------------
		// operators
		// ---------------------------------------------

		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_pod<tquat<precision_t>>::value, "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }


		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, 4> value;
	};

	using quat	= tquat<float>;
	using dquat   = tquat<double>;
	using iquat   = tquat<int>;
	using quat_sz = tquat<size_t>;


	template <typename precision_t>
	const tquat<precision_t> tquat<precision_t>::identity{0, 0, 0, 1};
} // namespace psl

namespace psl
{
	// ---------------------------------------------
	// operators tquat<precision_t>
	// ---------------------------------------------
#ifdef USE_SSE2

	template <typename precision_t>
	constexpr tquat<precision_t>& operator+=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_add_ps(_mm_load_ps(owner.value.data()), _mm_load_ps(other.value.data())));
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_add_pd(_mm_load_pd(owner.value.data()), _mm_load_pd(other.value.data())));
		}
		else
		{
			owner.value[0] += other.value[0];
			owner.value[1] += other.value[1];
			owner.value[2] += other.value[2];
			owner.value[3] += other.value[3];
		}
		return owner;
	}

	template <typename precision_t>
	constexpr tquat<precision_t>& operator*=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_mul_ps(_mm_load_ps(owner.value.data()), _mm_load_ps(other.value.data())));
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_mul_pd(_mm_load_pd(owner.value.data()), _mm_load_pd(other.value.data())));
		}
		else
		{
			owner.value[0] *= other.value[0];
			owner.value[1] *= other.value[1];
			owner.value[2] *= other.value[2];
			owner.value[3] *= other.value[3];
		}
		return owner;
	}


	template <typename precision_t>
	constexpr tquat<precision_t>& operator/=(tquat<precision_t>& owner, const tquat<precision_t>& other)
	{
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0 || other.value[3] == 0)
			throw std::runtime_exception("division by 0");
#endif
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_div_ps(_mm_load_ps(owner.value.data()), _mm_load_ps(other.value.data())));
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_div_pd(_mm_load_pd(owner.value.data()), _mm_load_pd(other.value.data())));
		}
		else
		{
			owner.value[0] /= other.value[0];
			owner.value[1] /= other.value[1];
			owner.value[2] /= other.value[2];
			owner.value[3] /= other.value[3];
		}
		return owner;
	}
	template <typename precision_t>
	constexpr tquat<precision_t>& operator-=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		if constexpr(std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(),
						 _mm_sub_ps(_mm_load_ps(owner.value.data()), _mm_load_ps(other.value.data())));
		}
		else if constexpr(std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(),
						 _mm_sub_pd(_mm_load_pd(owner.value.data()), _mm_load_pd(other.value.data())));
		}
		else
		{
			owner.value[0] -= other.value[0];
			owner.value[1] -= other.value[1];
			owner.value[2] -= other.value[2];
			owner.value[3] -= other.value[3];
		}
		return owner;
	}
#else
	template <typename precision_t>
	constexpr tquat<precision_t>& operator+=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		owner.value[0] += other.value[0];
		owner.value[1] += other.value[1];
		owner.value[2] += other.value[2];
		owner.value[3] += other.value[3];
		return owner;
	}

	template <typename precision_t>
	constexpr tquat<precision_t>& operator*=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		owner.value[0] *= other.value[0];
		owner.value[1] *= other.value[1];
		owner.value[2] *= other.value[2];
		owner.value[3] *= other.value[3];
		return owner;
	}


	template <typename precision_t>
	constexpr tquat<precision_t>& operator/=(tquat<precision_t>& owner, const tquat<precision_t>& other)
	{
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0 || other.value[3] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		owner.value[2] /= other.value[2];
		owner.value[3] /= other.value[3];
		return owner;
	}
	template <typename precision_t>
	constexpr tquat<precision_t>& operator-=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept
	{
		owner.value[0] -= other.value[0];
		owner.value[1] -= other.value[1];
		owner.value[2] -= other.value[2];
		owner.value[3] -= other.value[3];
		return owner;
	}
#endif
	template <typename precision_t>
	constexpr tquat<precision_t>& operator*=(tquat<precision_t>& owner, const precision_t& other) noexcept
	{
		owner.value[0] *= other;
		owner.value[1] *= other;
		owner.value[2] *= other;
		owner.value[3] *= other;
		return owner;
	}


	template <typename precision_t>
	constexpr tquat<precision_t>& operator/=(tquat<precision_t>& owner, const precision_t& other)
	{
		#ifdef MATH_DIV_ZERO_CHECK
		if (other == 0)
			throw std::runtime_exception("division by 0");
		#endif
		owner.value[0] /= other;
		owner.value[1] /= other;
		owner.value[2] /= other;
		owner.value[3] /= other;
		return owner;
	}

	template <typename precision_t>
	constexpr tquat<precision_t> operator+(const tquat<precision_t>& left, const tquat<precision_t>& right) noexcept
	{
		auto cpy = left;
		return cpy;
	}
	template <typename precision_t>
	constexpr tquat<precision_t> operator*(const tquat<precision_t>& left, const tquat<precision_t>& right) noexcept
	{
		auto cpy = left;
		cpy *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tquat<precision_t> operator/(const tquat<precision_t>& left, const tquat<precision_t>& right)
	{
		auto cpy = left;
		cpy /= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tquat<precision_t> operator*(const tquat<precision_t>& left, const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tquat<precision_t> operator/(const tquat<precision_t>& left, const precision_t& right)
	{
		auto cpy = left;
		cpy /= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tquat<precision_t> operator-(const tquat<precision_t>& left, const tquat<precision_t>& right) noexcept
	{
		auto cpy = left;
		cpy -= right;
		return cpy;
	}


} // namespace psl

namespace psl::math
{
	template <typename precision_t>
	constexpr static tquat<precision_t> cross(const tquat<precision_t>& left, const tquat<precision_t>& right) noexcept
	{
		return tquat<precision_t>{left[3] * right[3] - left[0] * right[0] - left[1] * right[1] - left[2] * right[2],
								  left[3] * right[0] + left[0] * right[3] + left[1] * right[2] - left[2] * right[1],
								  left[3] * right[1] + left[1] * right[3] + left[2] * right[0] - left[0] * right[2],
								  left[3] * right[2] + left[2] * right[3] + left[0] * right[1] - left[1] * right[0]};
	}
	template <typename precision_t>
	constexpr static precision_t dot(const tquat<precision_t>& left, const tquat<precision_t>& right) noexcept
	{
		return precision_t{left[0] * right[0] + left[1] * right[1] + left[2] * right[2] + left[3] * right[3]};
	}

	template <typename precision_t>
	constexpr static precision_t square_magnitude(const tquat<precision_t>& quat) noexcept
	{
		return dot(quat, quat);
	}

	template <typename precision_t>
	constexpr static precision_t magnitude(const tquat<precision_t>& quat) noexcept
	{
		return std::sqrt(dot(quat, quat));
	}


	template <typename precision_t>
	constexpr static tquat<precision_t> pow(const tquat<precision_t>& quat, precision_t pow_value = precision_t{2}) noexcept
	{
		return tquat<precision_t>
		{
			std::pow(quat[0], pow_value), std::pow(quat[1], pow_value), std::pow(quat[2], pow_value),
				std::pow(quat[3], pow_value)
		}
	}

	template <typename precision_t>
	constexpr static tquat<precision_t> sqrt(const tquat<precision_t>& quat) noexcept
	{
		return tquat<precision_t> { std::sqrt(quat[0]), std::sqrt(quat[1]), std::sqrt(quat[2]), std::sqrt(quat[3]) }
	}

	template <typename precision_t>
	constexpr static tquat<precision_t> normalize(const tquat<precision_t>& quat)
	{
		return quat / magnitude(quat);
	}
} // namespace psl::math