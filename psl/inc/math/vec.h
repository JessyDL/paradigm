#pragma once
#include <limits>
#include <utility>
#include "template_utils.h"
#include <algorithm>

#define USE_SSE2

namespace psl
{
	template <typename precision_t, size_t dimensions>
	struct tvec
	{
		using tvec_t = tvec<precision_t, dimensions>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;

		constexpr tvec(const std::array<precision_t, dimensions>& value) noexcept : value(value){};
		constexpr tvec(const precision_t& value) noexcept : value(utility::templates::make_array<dimensions>(value)){};

		template <typename... Args>
		constexpr tvec(Args&&... args) noexcept : value({static_cast<precision_t>(args)...}){};



		// ---------------------------------------------
		// operators
		// ---------------------------------------------
		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_pod<tvec<precision_t, dimensions>>::value, "should remain POD");

			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template<size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < dimensions, "out of range");
			return value.at(index);
		}
		template<size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < dimensions, "out of range");
			return value.at(index);
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, dimensions> value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 1>
	{
		using tvec_t = tvec<precision_t, 1>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& value) noexcept : value(value){};
		constexpr tvec(precision_t&& value) noexcept : value(std::move(value)){};

		// ---------------------------------------------
		// operators
		// ---------------------------------------------
		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_pod<tvec<precision_t, 1>>::value, "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template<size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < 1, "out of range");
			return value.at(index);
		}
		template<size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < 1, "out of range");
			return value.at(index);
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		precision_t value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 2>
	{
		using tvec_t = tvec<precision_t, 2>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t up;
		const static tvec_t down;
		const static tvec_t left;
		const static tvec_t right;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& x, const precision_t& y) noexcept : value({x, y}){};
		constexpr tvec(precision_t&& x, precision_t&& y) noexcept : value({std::move(x), std::move(y)}){};
		constexpr tvec(const precision_t& value) noexcept : value({value, value}){};

		// ---------------------------------------------
		// getters
		// ---------------------------------------------
		constexpr precision_t& x() noexcept { return value[0]; }
		constexpr const precision_t& x() const noexcept { return value[0]; }
		constexpr precision_t& y() noexcept { return value[1]; }
		constexpr const precision_t& y() const noexcept { return value[1]; }

		// ---------------------------------------------
		// operators
		// ---------------------------------------------
		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_pod<tvec<precision_t, 2>>::value, "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template<size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < 2, "out of range");
			return value.at(index);
		}
		template<size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < 2, "out of range");
			return value.at(index);
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, 2> value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 3>
	{
		using tvec_t = tvec<precision_t, 3>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t up;
		const static tvec_t down;
		const static tvec_t left;
		const static tvec_t right;
		const static tvec_t forward;
		const static tvec_t back;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& x, const precision_t& y, const precision_t& z) noexcept : value({x, y, z}){};
		constexpr tvec(precision_t&& x, precision_t&& y, precision_t&& z) noexcept
			: value({std::move(x), std::move(y), std::move(z)}){};
		constexpr tvec(const precision_t& value) noexcept : value({value, value, value}){};

		// ---------------------------------------------
		// getters
		// ---------------------------------------------
		constexpr precision_t& x() noexcept { return value[0]; }
		constexpr const precision_t& x() const noexcept { return value[0]; }
		constexpr precision_t& y() noexcept { return value[1]; }
		constexpr const precision_t& y() const noexcept { return value[1]; }
		constexpr precision_t& z() noexcept { return value[2]; }
		constexpr const precision_t& z() const noexcept { return value[2]; }

		// ---------------------------------------------
		// operators
		// ---------------------------------------------

		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_pod<tvec<precision_t, 3>>::value, "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template<size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < 3, "out of range");
			return value.at(index);
		}
		template<size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < 3, "out of range");
			return value.at(index);
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, 3> value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 4>
	{
		using tvec_t = tvec<precision_t, 4>;

		const static tvec_t zero;
		const static tvec_t one;
		const static tvec_t up;
		const static tvec_t down;
		const static tvec_t left;
		const static tvec_t right;
		const static tvec_t forward;
		const static tvec_t back;
		const static tvec_t position;
		const static tvec_t direction;
		const static tvec_t infinity;
		const static tvec_t negative_infinity;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& x, const precision_t& y, const precision_t& z, const precision_t& w) noexcept
			: value({x, y, z, w}){};
		constexpr tvec(precision_t&& x, precision_t&& y, precision_t&& z, precision_t&& w) noexcept
			: value({std::move(x), std::move(y), std::move(z), std::move(w)}){};
		constexpr tvec(const precision_t& value) noexcept : value({value, value, value, value}){};

		template<typename src_precision_t>
		constexpr tvec(std::array<src_precision_t,4>&& arr) noexcept
			: value({std::move(arr)}){};

		template<typename src_precision_t>
		constexpr tvec(const std::array<src_precision_t,4>& value) noexcept : value({value}){};

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
			static_assert(std::is_pod<tvec<precision_t, 4>>::value, "should remain POD");
			return value[index];
		}

		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }

		template<size_t index>
		constexpr precision_t& at() noexcept
		{
			static_assert(index < 4, "out of range");
			return value.at(index);
		}
		template<size_t index>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index < 4, "out of range");
			return value.at(index);
		}


		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, 4> value;
	};


	using vec1 = psl::tvec<float, 1>;
	using vec2 = psl::tvec<float, 2>;
	using vec3 = psl::tvec<float, 3>;
	using vec4 = psl::tvec<float, 4>;

	using dvec1 = psl::tvec<double, 1>;
	using dvec2 = psl::tvec<double, 2>;
	using dvec3 = psl::tvec<double, 3>;
	using dvec4 = psl::tvec<double, 4>;

	using ivec1 = psl::tvec<int, 1>;
	using ivec2 = psl::tvec<int, 2>;
	using ivec3 = psl::tvec<int, 3>;
	using ivec4 = psl::tvec<int, 4>;

	using vec1_sz = psl::tvec<size_t, 1>;
	using vec2_sz = psl::tvec<size_t, 2>;
	using vec3_sz = psl::tvec<size_t, 3>;
	using vec4_sz = psl::tvec<size_t, 4>;


	template<typename precision_t, size_t dimensions>
	const tvec<precision_t, dimensions>  tvec<precision_t, dimensions>::zero{ 0 };
	template<typename precision_t, size_t dimensions>
	const tvec<precision_t, dimensions>  tvec<precision_t, dimensions>::one{ 1 };
	template<typename precision_t, size_t dimensions>
	const tvec<precision_t, dimensions>  tvec<precision_t, dimensions>::infinity{ std::numeric_limits<precision_t>::infinity() };
	template<typename precision_t, size_t dimensions>
	const tvec<precision_t, dimensions>  tvec<precision_t, dimensions>::negative_infinity{ -std::numeric_limits<precision_t>::infinity() };


	template<typename precision_t>
	const tvec<precision_t, 2>  tvec<precision_t, 2>::zero{ 0 };
	template<typename precision_t>
	const tvec<precision_t, 2>  tvec<precision_t, 2>::one{ 1 };
	template<typename precision_t>
	const tvec<precision_t, 2>  tvec<precision_t, 2>::infinity{ std::numeric_limits<precision_t>::infinity() };
	template<typename precision_t>
	const tvec<precision_t, 2>  tvec<precision_t, 2>::negative_infinity{ -std::numeric_limits<precision_t>::infinity() };
	template<typename precision_t>
	const tvec<precision_t, 2>  tvec<precision_t, 2>::up{ 0,1 };
	template<typename precision_t>
	const tvec<precision_t, 2>  tvec<precision_t, 2>::down{ 0,-1};
	template<typename precision_t>
	const tvec<precision_t, 2>  tvec<precision_t, 2>::left{ -1, 0 };
	template<typename precision_t>
	const tvec<precision_t, 2>  tvec<precision_t, 2>::right{ 1,0};

	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::zero{ 0 };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::one{ 1 };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::infinity{ std::numeric_limits<precision_t>::infinity() };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::negative_infinity{ -std::numeric_limits<precision_t>::infinity() };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::up{ 0,1,0 };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::down{ 0,-1,0 };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::left{ -1, 0, 0 };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::right{ 1,0,0 };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::forward{ 0,0,1 };
	template<typename precision_t>
	const tvec<precision_t, 3>  tvec<precision_t, 3>::back{ 0,0,-1 };

	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::zero{ 0 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::one{ 1 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::infinity{ std::numeric_limits<precision_t>::infinity() };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::negative_infinity{ -std::numeric_limits<precision_t>::infinity() };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::up{ 0,1,0, 0 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::down{ 0,-1,0,0 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::left{ -1, 0, 0,0 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::right{ 1,0,0,0 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::forward{ 0,0,1,0 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::back{ 0,0,-1,0 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::position{ 0,0,0,1 };
	template<typename precision_t>
	const tvec<precision_t, 4>  tvec<precision_t, 4>::direction{ 0 };
} // namespace psl


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
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value /= other.value;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator*=(tvec<precision_t, 1>& owner, const precision_t& other) noexcept
	{
		owner.value *= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1>& operator/=(tvec<precision_t, 1>& owner, const precision_t& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value /= other;
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
	constexpr tvec<precision_t, 1> operator*(const tvec<precision_t, 1>& left,
		const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 1> operator/(const tvec<precision_t, 1>& left,
		const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value /= right;
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

	template <typename precision_t>
	constexpr bool operator==(const tvec<precision_t, 1>& left,
		const tvec<precision_t, 1>& right) noexcept
	{
		return left[0] == right[0];
	}

	template <typename precision_t>
	constexpr bool operator!=(const tvec<precision_t, 1>& left,
		const tvec<precision_t, 1>& right) noexcept
	{
		return left[0] != right[0];
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
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0 || other.value[1] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator*=(tvec<precision_t, 2>& owner, const precision_t& other) noexcept
	{
		owner.value[0] *= other;
		owner.value[1] *= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2>& operator/=(tvec<precision_t, 2>& owner, const precision_t& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0 || other.value[1] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other;
		owner.value[1] /= other;
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
	constexpr tvec<precision_t, 2> operator*(const tvec<precision_t, 2>& left,
		const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] *= right;
		cpy.value[1] *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 2> operator/(const tvec<precision_t, 2>& left,
		const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] /= right;
		cpy.value[1] /= right;
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

	template <typename precision_t>
	constexpr bool operator==(const tvec<precision_t, 2>& left,
		const tvec<precision_t, 2>& right) noexcept
	{
		return left[0] == right[0] &&
			left[1] == right[1];
	}

	template <typename precision_t>
	constexpr bool operator!=(const tvec<precision_t, 2>& left,
		const tvec<precision_t, 2>& right) noexcept
	{
		return left[0] != right[0] ||
			left[1] != right[1];
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
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		owner.value[2] /= other.value[2];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator*=(tvec<precision_t, 3>& owner, const precision_t& other) noexcept
	{
		owner.value[0] *= other;
		owner.value[1] *= other;
		owner.value[2] *= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3>& operator/=(tvec<precision_t, 3>& owner, const precision_t& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0)
			throw std::runtime_exception("division by 0");
#endif
		owner.value[0] /= other;
		owner.value[1] /= other;
		owner.value[2] /= other;
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
	constexpr tvec<precision_t, 3> operator*(const tvec<precision_t, 3>& left,
		const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] *= right;
		cpy.value[1] *= right;
		cpy.value[2] *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 3> operator/(const tvec<precision_t, 3>& left,
		const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy.value[0] /= right;
		cpy.value[1] /= right;
		cpy.value[2] /= right;
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

	template <typename precision_t>
	constexpr bool operator==(const tvec<precision_t, 3>& left,
		const tvec<precision_t, 3>& right) noexcept
	{
		return left[0] == right[0] &&
			left[1] == right[1] &&
			left[2] == right[2];
	}

	template <typename precision_t>
	constexpr bool operator!=(const tvec<precision_t, 3>& left,
		const tvec<precision_t, 3>& right) noexcept
	{
		return left[0] != right[0] ||
			left[1] != right[1] ||
			left[2] != right[2];
	}

	// ---------------------------------------------
	// operators tvec<precision_t, 4>
	// ---------------------------------------------

#ifdef USE_SSE2
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
#ifdef MATH_DIV_ZERO_CHECK
		if (other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0 || other.value[3] == 0)
			throw std::runtime_exception("division by 0");
#endif
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
#else
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator+=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		owner.value[0] += other.value[0];
		owner.value[1] += other.value[1];
		owner.value[2] += other.value[2];
		owner.value[3] += other.value[3];
		return owner;
	}

	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator*=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		owner.value[0] *= other.value[0];
		owner.value[1] *= other.value[1];
		owner.value[2] *= other.value[2];
		owner.value[3] *= other.value[3];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator/=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		owner.value[0] /= other.value[0];
		owner.value[1] /= other.value[1];
		owner.value[2] /= other.value[2];
		owner.value[3] /= other.value[3];
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator-=(tvec<precision_t, 4>& owner, const tvec<precision_t, 4>& other) noexcept
	{
		owner.value[0] -= other.value[0];
		owner.value[1] -= other.value[1];
		owner.value[2] -= other.value[2];
		owner.value[3] -= other.value[3];
		return owner;
	}
#endif

	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator*=(tvec<precision_t, 4>& owner, const precision_t& other) noexcept
	{
		owner.value[0] *= other;
		owner.value[1] *= other;
		owner.value[2] *= other;
		owner.value[3] *= other;
		return owner;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4>& operator/=(tvec<precision_t, 4>& owner, const precision_t& other) noexcept
	{
		owner.value[0] /= other;
		owner.value[1] /= other;
		owner.value[2] /= other;
		owner.value[3] /= other;
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
	constexpr tvec<precision_t, 4> operator*(const tvec<precision_t, 4>& left,
		const precision_t& right) noexcept
	{
		auto cpy = left;
		cpy *= right;
		return cpy;
	}
	template <typename precision_t>
	constexpr tvec<precision_t, 4> operator/(const tvec<precision_t, 4>& left,
		const precision_t& right) noexcept
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


	template <typename precision_t>
	constexpr bool operator==(const tvec<precision_t, 4>& left,
		const tvec<precision_t, 4>& right) noexcept
	{
		return left[0] == right[0] &&
			left[1] == right[1] &&
			left[2] == right[2] &&
			left[3] == right[3];
	}

	template <typename precision_t>
	constexpr bool operator!=(const tvec<precision_t, 4>& left,
		const tvec<precision_t, 4>& right) noexcept
	{
		return left[0] != right[0] ||
			left[1] != right[1] ||
			left[2] != right[2] ||
			left[3] != right[3];
	}

	// ---------------------------------------------
	// operators tvec<precision_t, N>
	// ---------------------------------------------
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator+=(tvec<precision_t, dimensions>& owner,
		const tvec<precision_t, dimensions>& other) noexcept
	{
		for (size_t i = 0; i < dimensions; ++i) owner.value[i] += other.value[i];
		return owner;
	}

	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator*=(tvec<precision_t, dimensions>& owner,
		const tvec<precision_t, dimensions>& other) noexcept
	{
		for (size_t i = 0; i < dimensions; ++i) owner.value[i] *= other.value[i];
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator/=(tvec<precision_t, dimensions>& owner,
		const tvec<precision_t, dimensions>& other) noexcept
	{
		for (size_t i = 0; i < dimensions; ++i)
		{
#ifdef MATH_DIV_ZERO_CHECK
			if (other.value[i] == 0)
				throw std::runtime_exception("division by 0");
#endif
			owner.value[i] /= other.value[i];
		}
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator*=(tvec<precision_t, dimensions>& owner,
		const precision_t& other) noexcept
	{
		for (size_t i = 0; i < dimensions; ++i) owner.value[i] *= other;
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator/=(tvec<precision_t, dimensions>& owner,
		const precision_t& other) noexcept
	{
#ifdef MATH_DIV_ZERO_CHECK
		if (other == 0)
			throw std::runtime_exception("division by 0");
#endif
		for (size_t i = 0; i < dimensions; ++i)
		{
			owner.value[i] /= other;
		}
		return owner;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions>& operator-=(tvec<precision_t, dimensions>& owner,
		const tvec<precision_t, dimensions>& other) noexcept
	{
		for (size_t i = 0; i < dimensions; ++i) owner.value[i] -= other.value[i];
		return owner;
	}

	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator+(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for (size_t i = 0; i < dimensions; ++i) cpy.value[i] += right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator*(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for (size_t i = 0; i < dimensions; ++i) cpy.value[i] *= right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator/(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for (size_t i = 0; i < dimensions; ++i) cpy.value[i] /= right.value[i];
		return cpy;
	}
	template <typename precision_t, size_t dimensions>
	constexpr tvec<precision_t, dimensions> operator-(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		auto cpy = left;
		for (size_t i = 0; i < dimensions; ++i) cpy.value[i] -= right.value[i];
		return cpy;
	}



	template <typename precision_t, size_t dimensions>
	constexpr bool operator==(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		return std::memcmp(left.value.data(), right.value.data(), dimensions * sizeof(precision_t)) == 0;
	}

	template <typename precision_t, size_t dimensions>
	constexpr bool operator!=(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		return std::memcmp(left.value.data(), right.value.data(), dimensions * sizeof(precision_t)) != 0;
	}

}
namespace psl::math
{
	// ---------------------------------------------
	// operations between tvec<precision_t, N>
	// ---------------------------------------------
	template <typename precision_t, size_t dimensions>
	constexpr static precision_t dot(const tvec<precision_t, dimensions>& left,
		const tvec<precision_t, dimensions>& right) noexcept
	{
		precision_t res = precision_t{0};
		for (size_t i = 0; i < dimensions; ++i)
			res += left[i] * right[i];
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static precision_t square_magnitude(const tvec<precision_t, dimensions>& vec) noexcept
	{
		precision_t res = precision_t{0};
		for (size_t i = 0; i < dimensions; ++i)
			res += vec[i] * vec[i];
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static precision_t magnitude(const tvec<precision_t, dimensions>& vec) noexcept
	{
		precision_t res = precision_t{0};
		for (size_t i = 0; i < dimensions; ++i)
			res += vec[i] * vec[i];
		return std::sqrt(res);
	}
	template <typename precision_t, size_t dimensions>
	constexpr static tvec<precision_t, dimensions> sqrt(const tvec<precision_t, dimensions>& vec) noexcept
	{
		tvec<precision_t, dimensions> res{};
		for (size_t i = 0; i < dimensions; ++i)
		{
			res[i] = std::sqrt(vec[i]);
		}
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static tvec<precision_t, dimensions> pow(const tvec<precision_t, dimensions>& vec, precision_t pow_value = precision_t{ 2 }) noexcept
	{
		tvec<precision_t, dimensions> res{};
		for (size_t i = 0; i < dimensions; ++i)
		{
			res[i] = std::pow(vec[i], pow_value);
		}
		return res;
	}

	template <typename precision_t, size_t dimensions>
	constexpr static tvec<precision_t, dimensions> normalize(const tvec<precision_t, dimensions>& vec) noexcept
	{
		return res / magnitude(vec);
	}

	template <typename precision_t>
	constexpr static tvec<precision_t, 3> cross(const tvec<precision_t, 3>& left,
		const tvec<precision_t, 3>& right) noexcept
	{
		return tvec<precision_t, 3>
		{
			left[1] * right[2] - left[2]*right[1],
			left[2] * right[0] - left[0]*right[2],
			left[0] * right[1] - left[1]*right[0]
		};
	}


} // namespace psl