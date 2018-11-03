
namespace psl
{
	// ---------------------------------------------
	// operators tvec<precision_t, 1>
	// ---------------------------------------------
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator+=(tvec<precision_t, 1>& owner, const tvec<precision_t, 1>& other) noexcept
	{
		owner.value += other.value;
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator*=(tvec<precision_t, 1>& owner, const tvec<precision_t, 1>& other) noexcept
	{
		owner.value *= other.value;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator/=(tvec<precision_t, 1>& owner, const tvec<precision_t, 1>& other) noexcept
	{
		owner.value /= other.value;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator-=(tvec<precision_t, 1>& owner, const tvec<precision_t, 1>& other) noexcept
	{
		owner.value -= other.value;
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator+(const tvec<precision_t, 1>& left,
		const tvec<precision_t, 1>& right) noexcept
	{
		auto cpy = left;
		cpy.value += right.value;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator*(const tvec<precision_t, 1>& left,
		const tvec<precision_t, 1>& right) noexcept
	{
		auto cpy = left;
		cpy.value *= right.value;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator/(const tvec<precision_t, 1>& left,
		const tvec<precision_t, 1>& right) noexcept
	{
		auto cpy = left;
		cpy.value /= right.value;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator-(const tvec<precision_t, 1>& left,
		const tvec<precision_t, 1>& right) noexcept
	{
		auto cpy = left;
		cpy.value -= right.value;
		return cpy;
	}

	// ---------------------------------------------
	// operators tvec<precision_t, 2>
	// ---------------------------------------------
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator+=(tvec<precision_t, 2>& owner, const tvec<precision_t, 2>& other) noexcept
	{
		owner.value[0] += other.value[0];
		owner.value[1] += other.value[1];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator*=(tvec<precision_t, 2>& owner, const tvec<precision_t, 2>& other) noexcept
	{
		owner.value[0] *= other.value[0];
		owner.value[1] *= other.value[1];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator/=(tvec<precision_t, 2>& owner, const tvec<precision_t, 2>& other) noexcept
	{
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator-=(tvec<precision_t, 2>& owner, const tvec<precision_t, 2>& other) noexcept
	{
		owner.value[0] -= other.value[0];
		owner.value[1] -= other.value[1];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator+(const tvec<precision_t, 2>& left,
		const tvec<precision_t, 2>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] += right.value[0];
		cpy.value[1] += right.value[1];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator*(const tvec<precision_t, 2>& left,
		const tvec<precision_t, 2>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] *= right.value[0];
		cpy.value[1] *= right.value[1];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator/(const tvec<precision_t, 2>& left,
		const tvec<precision_t, 2>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] /= right.value[0];
		cpy.value[1] /= right.value[1];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator-(const tvec<precision_t, 2>& left,
		const tvec<precision_t, 2>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] -= right.value[0];
		cpy.value[1] -= right.value[1];
		return cpy;
	}

	// ---------------------------------------------
	// operators tvec<precision_t, 3>
	// ---------------------------------------------
	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator+=(tvec<precision_t, 3>& owner, const tvec<precision_t, 3>& other) noexcept
	{
		owner.value[0] += other.value[0];
		owner.value[1] += other.value[1];
		owner.value[2] += other.value[2];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator*=(tvec<precision_t, 3>& owner, const tvec<precision_t, 3>& other) noexcept
	{
		owner.value[0] *= other.value[0];
		owner.value[1] *= other.value[1];
		owner.value[2] *= other.value[2];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator/=(tvec<precision_t, 3>& owner, const tvec<precision_t, 3>& other) noexcept
	{
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		owner.value[2] /= other.value[2];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator-=(tvec<precision_t, 3>& owner, const tvec<precision_t, 3>& other) noexcept
	{
		owner.value[0] -= other.value[0];
		owner.value[1] -= other.value[1];
		owner.value[2] -= other.value[2];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator+(const tvec<precision_t, 3>& left,
		const tvec<precision_t, 3>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] += right.value[0];
		cpy.value[1] += right.value[1];
		cpy.value[2] += right.value[2];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator*(const tvec<precision_t, 3>& left,
		const tvec<precision_t, 3>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] *= right.value[0];
		cpy.value[1] *= right.value[1];
		cpy.value[2] *= right.value[2];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator/(const tvec<precision_t, 3>& left,
		const tvec<precision_t, 3>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] /= right.value[0];
		cpy.value[1] /= right.value[1];
		cpy.value[2] /= right.value[2];
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator-(const tvec<precision_t, 3>& left,
		const tvec<precision_t, 3>& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] -= right.value[0];
		cpy.value[1] -= right.value[1];
		cpy.value[2] -= right.value[2];
		return cpy;
	}

	// ---------------------------------------------
	// operators tvec<precision_t, 4>
	// ---------------------------------------------
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator+=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		if constexpr (std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(), 
				_mm_add_ps(
					_mm_load_ps(owner.value.data()),
					_mm_load_ps(other.value.data())
				)
			);
		}
		else if constexpr (std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(), 
				_mm_add_pd(
					_mm_load_pd(owner.value.data()),
					_mm_load_pd(other.value.data())
				)
			);
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
	constexpr tvec<precision_t, 4>& operator*=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		if constexpr (std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(), 
				_mm_mul_ps(
					_mm_load_ps(owner.value.data()),
					_mm_load_ps(other.value.data())
				)
			);
		}
		else if constexpr (std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(), 
				_mm_mul_pd(
					_mm_load_pd(owner.value.data()),
					_mm_load_pd(other.value.data())
				)
			);
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
	constexpr tvec<precision_t, 4>& operator/=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		if constexpr (std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(), 
				_mm_div_ps(
					_mm_load_ps(owner.value.data()),
					_mm_load_ps(other.value.data())
				)
			);
		}
		else if constexpr (std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(), 
				_mm_div_pd(
					_mm_load_pd(owner.value.data()),
					_mm_load_pd(other.value.data())
				)
			);
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
	constexpr tvec<precision_t, 4>& operator-=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		if constexpr (std::is_same<float, precision_t>::value)
		{
			_mm_store_ps(owner.value.data(), 
				_mm_sub_ps(
					_mm_load_ps(owner.value.data()),
					_mm_load_ps(other.value.data())
				)
			);
		}
		else if constexpr (std::is_same<double, precision_t>::value)
		{
			_mm_store_pd(owner.value.data(), 
				_mm_sub_pd(
					_mm_load_pd(owner.value.data()),
					_mm_load_pd(other.value.data())
				)
			);
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

	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator+(const tvec<precision_t, 4>& left,
		const tvec<precision_t, 4>& right) noexcept
	{
		auto cpy = left;
		cpy += right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator*(const tvec<precision_t, 4>& left,
		const tvec<precision_t, 4>& right) noexcept
	{
		auto cpy = left;
		cpy *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator/(const tvec<precision_t, 4>& left,
		const tvec<precision_t, 4>& right) noexcept
	{
		auto cpy = left;
		cpy /= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator-(const tvec<precision_t, 4>& left,
		const tvec<precision_t, 4>& right) noexcept
	{
		auto cpy = left;
		cpy -= right;
		return cpy;
	}


	// ---------------------------------------------
	// operators tvec<precision_t, N>
	// ---------------------------------------------
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator+=(tvec<precision_t, dimensions>& owner,
		const tvec<precision_t, dimensions>& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i) owner.value[i] += other.value[i];
		return owner;
	}

	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator*=(tvec<precision_t, dimensions>& owner,
		const tvec<precision_t, dimensions>& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i) owner.value[i] *= other.value[i];
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator/=(tvec<precision_t, dimensions>& owner,
		const tvec<precision_t, dimensions>& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i) owner.value[i] /= other.value[i];
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator-=(tvec<precision_t, dimensions>& owner,
		const tvec<precision_t, dimensions>& other) noexcept
	{
		for(size_t i = 0; i < dimensions; ++i) owner.value[i] -= other.value[i];
		return owner;
	}

	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator+(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] += right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator*(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] *= right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator/(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] /= right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator-(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for(size_t i = 0; i < dimensions; ++i) cpy.value[i] -= right.value[i];
		return cpy;
	}
} // namespace psl