#pragma once
#include <utility>
#include <tuple>

#define USE_SSE2

namespace psl
{
	namespace details
	{
		template <size_t, class T>
		using T_ = T;

		template <class T, size_t... Is>
		auto make_tuple(std::index_sequence<Is...>)
		{
			return std::tuple<T_<Is, T>...>{};
		}

		template <class T, size_t N>
		auto make_tuple()
		{
			return make_tuple<T>(std::make_index_sequence<N>{});
		}


	} // namespace details
	template <typename precision_t, size_t dimensions>
	struct tvec
	{
		using tvec_t = tvec<precision_t, dimensions>;

		constexpr tvec() noexcept = default;

		constexpr tvec(const std::array<precision_t, dimensions>& value) noexcept : value(value){};

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

		constexpr precision_t& operator[](size_t index) const noexcept { return value[index]; }


		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, dimensions> value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 1>
	{
		using tvec_t = tvec<precision_t, 1>;

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

		constexpr precision_t& operator[](size_t index) const noexcept { return value[index]; }

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		precision_t value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 2>
	{
		using tvec_t = tvec<precision_t, 2>;

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

		constexpr precision_t& operator[](size_t index) const noexcept { return value[index]; }

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, 2> value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 3>
	{
		using tvec_t = tvec<precision_t, 3>;

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

		constexpr precision_t& operator[](size_t index) const noexcept { return value[index]; }

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, 3> value;
	};

	template <typename precision_t>
	struct tvec<precision_t, 4>
	{
		using tvec_t = tvec<precision_t, 4>;

		constexpr tvec() noexcept = default;
		constexpr tvec(const precision_t& x, const precision_t& y, const precision_t& z, const precision_t& w) noexcept
			: value({x, y, z, w}){};
		constexpr tvec(precision_t&& x, precision_t&& y, precision_t&& z, precision_t&& w) noexcept
			: value({std::move(x), std::move(y), std::move(z), std::move(w)}){};
		constexpr tvec(const precision_t& value) noexcept : value({value, value, value, value}){};

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

		constexpr precision_t& operator[](size_t index) const noexcept { return value[index]; }


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
} // namespace psl

#ifdef USE_SSE2
#include "vec_simd.inl"
#else
#include "vec.inl"
#endif