#pragma once
#include <utility>
#include <tuple>

namespace psl
{
	namespace details
	{
		template<size_t, class T>
		using T_ = T;

		template<class T, size_t... Is>
		auto make_tuple(std::index_sequence<Is...>) { return std::tuple<T_<Is, T>...>{}; }

		template<class T, size_t N>
		auto make_tuple() { return make_tuple<T>(std::make_index_sequence<N>{}); } 


	}
	template<typename precision_t, size_t dimensions>
	struct tvec
	{
		using tvec_t = tvec<precision_t, dimensions>;

		tvec() noexcept = default;

		constexpr tvec(const std::array<precision_t, dimensions>& values) noexcept : values(values) {};

		template<typename... Args>
		constexpr tvec(Args&&... args) noexcept : values( { static_cast<precision_t>(args)...} ) {};

		constexpr precision_t& operator[](size_t index) noexcept
		{
			static_assert(std::is_pod< tvec<precision_t, dimensions>>::value, "should remain POD");
			return values[index];
		}

		constexpr precision_t& operator[](size_t index) const noexcept
		{
			return values[index];
		}

		// ---------------------------------------------
		// getters
		// ---------------------------------------------
		constexpr precision_t& x() noexcept
		{
			return values[1];
		}
		constexpr precision_t& x() const noexcept
		{
			return values[1];
		}

		template<typename = std::enable_if<(dimensions >1)>::type>
		constexpr precision_t& y() noexcept
		{
			return values[1];
		}
		template<typename = std::enable_if<(dimensions >1)>::type>
		constexpr precision_t& y() const noexcept
		{
			return values[1];
		}

		template<typename = std::enable_if<(dimensions >2)>::type>
		constexpr precision_t& z() noexcept
		{
			return values[2];
		}
		template<typename = std::enable_if<(dimensions >2)>::type>
		constexpr precision_t& z() const noexcept
		{
			return values[2];
		}

		template<typename = std::enable_if<(dimensions >3)>::type>
		constexpr precision_t& w() noexcept
		{
			return values[3];
		}
		template<typename = std::enable_if<(dimensions >3)>::type>
		constexpr precision_t& w() const noexcept
		{
			return values[3];
		}


		// ---------------------------------------------
		// operators
		// ---------------------------------------------

		tvec_t& operator+=(const tvec_t& other) noexcept
		{
			for (size_t i = 0u; i < dimensions; ++i)
				values[i] += other.values[i];
			return *this;
		}
		tvec_t& operator*=(const tvec_t& other) noexcept
		{
			for (size_t i = 0u; i < dimensions; ++i)
				values[i] *= other.values[i];
			return *this;
		}
		tvec_t& operator/=(const tvec_t& other) noexcept
		{
			for (size_t i = 0u; i < dimensions; ++i)
				values[i] /= other.values[i];
			return *this;
		}
		tvec_t& operator-=(const tvec_t& other) noexcept
		{
			for (size_t i = 0u; i < dimensions; ++i)
				values[i] -= other.values[i];
			return *this;
		}

		tvec_t operator+(const tvec_t& other) const noexcept
		{
			auto cpy = *this;
			for (size_t i = 0u; i < dimensions; ++i)
				cpy[i] += other.values[i];
			return cpy;
		}
		tvec_t operator*(const tvec_t& other) const noexcept
		{
			auto cpy = *this;
			for (size_t i = 0u; i < dimensions; ++i)
				cpy[i] *= other.values[i];
			return cpy;
		}
		tvec_t operator/(const tvec_t& other) const noexcept
		{
			auto cpy = *this;
			for (size_t i = 0u; i < dimensions; ++i)
				cpy[i] /= other.values[i];
			return cpy;
		}
		tvec_t operator-(const tvec_t& other) const noexcept
		{
			auto cpy = *this;
			for (size_t i = 0u; i < dimensions; ++i)
				cpy[i] -= other.values[i];
			return cpy;
		}

		// ---------------------------------------------
		// members
		// ---------------------------------------------
		std::array<precision_t, dimensions> values;
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
}