#pragma once
#include "vec.h"
#include <algorithm>
#include "template_utils.h"

namespace psl
{
	template <typename precision_t, size_t columns_n, size_t rows_n>
	struct tmat
	{
		constexpr tmat() noexcept = default;
		template <typename... Args, typename = typename std::enable_if<sizeof...(Args) == columns_n * rows_n>::type>
		constexpr tmat(Args&&... args) noexcept : value({static_cast<precision_t>(std::forward<Args>(args))...}){};

		constexpr tmat(const precision_t& val) noexcept
			: value({0})
		{
			for(size_t i = 0; i < columns_n; ++i)
			{
				value[i * columns_n + i] = val;
			}
		};

		template <size_t columns2_n, size_t rows2_n>
		constexpr tmat(const tmat<precision_t, columns2_n, rows2_n>& val) noexcept : value({0})
		{
			for(size_t i = 0; i < columns_n; ++i)
			{
				value[i * columns_n + i] = 1;
			}
			for(size_t i = 0; i < rows2_n; ++i)
			{
				for(size_t x = 0; x < columns2_n; ++x)
				{
					value[x + columns_n * i] = val[x + columns2_n * i];				
				}
			}
		}
		constexpr precision_t& operator[](size_t index) noexcept { return value[index]; }
		constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }
		constexpr precision_t& operator[](std::array<size_t, 2> index) noexcept
		{
			return value[index[0] * columns_n + index[1]];
		}
		constexpr const precision_t& operator[](std::array<size_t, 2> index) const noexcept
		{
			return value[index[0] * columns_n + index[1]];
		}

		precision_t& operator()(size_t row, size_t column) { return value[row * columns_n + column]; }

		precision_t& operator()(size_t index) { return value[index]; }

		template <size_t row, size_t column>
		constexpr precision_t& at() noexcept
		{
			static_assert(row < rows_n && column < columns_n, "out of range");
			return value[column + columns_n * row];
		}
		template <size_t row, size_t column>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(row < rows_n && column < columns_n, "out of range");
			return value[column + columns_n * row];
		}

		template <size_t index>
		constexpr tvec<precision_t, columns_n> row() const noexcept
		{
			static_assert(index < rows_n, "out of range");
			tvec<precision_t, columns_n> res{};
			std::memcpy(res.value.data(), &value[index * columns_n], sizeof(precision_t) * columns_n);
			return res;
		}
		constexpr tvec<precision_t, columns_n> row(size_t index) const noexcept
		{
			tvec<precision_t, columns_n> res{};
			std::memcpy(res.value.data(), &value[index * columns_n], sizeof(precision_t) * columns_n);
			return res;
		}

		template <size_t index>
		constexpr tvec<precision_t, rows_n> column() const noexcept
		{
			static_assert(index < columns_n, "out of range");
			tvec<precision_t, rows_n> res{};
			for(size_t i = 0; i < rows_n; ++i) res[i] = value[i * rows_n + index];
			return res;
		}
		constexpr tvec<precision_t, rows_n> column(size_t index) const noexcept
		{
			tvec<precision_t, rows_n> res{};
			for(size_t i = 0; i < rows_n; ++i) res[i] = value[i * rows_n + index];
			return res;
		}

		template <size_t index>
		constexpr void row(const tvec<precision_t, columns_n>& v) noexcept
		{
			static_assert(index < rows_n, "out of range");
			std::memcpy(&value[index * columns_n], v.value.data(), sizeof(precision_t) * columns_n);
		}
		constexpr void row(size_t index, const tvec<precision_t, columns_n>& v) noexcept
		{
			std::memcpy(&value[index * columns_n], v.value.data(), sizeof(precision_t) * columns_n);
		}

		template <size_t index>
		constexpr void column(const tvec<precision_t, rows_n>& v) noexcept
		{
			static_assert(index < columns_n, "out of range");
			for(size_t i = 0; i < rows_n; ++i) value[i * rows_n + index] = v[i];
		}
		constexpr void column(size_t index, const tvec<precision_t, rows_n>& v) noexcept
		{
			for(size_t i = 0; i < rows_n; ++i) value[i * rows_n + index] = v[i];
		}

		constexpr void swizzle() noexcept
		{
			std::array<precision_t, columns_n * rows_n> new_values;
			for(size_t i = 0; i < columns_n; ++i)
				std::memcpy(&new_values[i * columns_n], column(i).value.data(), sizeof(precision_t) * columns_n);
			value = new_values;
		}


		std::array<precision_t, columns_n * rows_n> value;
	};

	using mat4x4	= tmat<float, 4, 4>;
	using dmat4x4   = tmat<double, 4, 4>;
	using imat4x4   = tmat<int, 4, 4>;
	using mat4x4_sz = tmat<size_t, 4, 4>;

	using mat4x3	= tmat<float, 4, 3>;
	using dmat4x3   = tmat<double, 4, 3>;
	using imat4x3   = tmat<int, 4, 3>;
	using mat4x3_sz = tmat<size_t, 4, 3>;


	using mat4x2	= tmat<float, 4, 2>;
	using dmat4x2   = tmat<double, 4, 2>;
	using imat4x2   = tmat<int, 4, 2>;
	using mat4x2_sz = tmat<size_t, 4, 2>;


	using mat4x1	= tmat<float, 4, 1>;
	using dmat4x1   = tmat<double, 4, 1>;
	using imat4x1   = tmat<int, 4, 1>;
	using mat4x1_sz = tmat<size_t, 4, 1>;


	using mat3x4	= tmat<float, 3, 4>;
	using dmat3x4   = tmat<double, 3, 4>;
	using imat3x4   = tmat<int, 3, 4>;
	using mat3x4_sz = tmat<size_t, 3, 4>;

	using mat3x3	= tmat<float, 3, 3>;
	using dmat3x3   = tmat<double, 3, 3>;
	using imat3x3   = tmat<int, 3, 3>;
	using mat3x3_sz = tmat<size_t, 3, 3>;


	using mat3x2	= tmat<float, 3, 2>;
	using dmat3x2   = tmat<double, 3, 2>;
	using imat3x2   = tmat<int, 3, 2>;
	using mat3x2_sz = tmat<size_t, 3, 2>;


	using mat3x1	= tmat<float, 3, 1>;
	using dmat3x1   = tmat<double, 3, 1>;
	using imat3x1   = tmat<int, 3, 1>;
	using mat3x1_sz = tmat<size_t, 3, 1>;

	using mat2x4	= tmat<float, 2, 4>;
	using dmat2x4   = tmat<double, 2, 4>;
	using imat2x4   = tmat<int, 2, 4>;
	using mat2x4_sz = tmat<size_t, 2, 4>;

	using mat2x3	= tmat<float, 2, 3>;
	using dmat2x3   = tmat<double, 2, 3>;
	using imat2x3   = tmat<int, 2, 3>;
	using mat2x3_sz = tmat<size_t, 2, 3>;


	using mat2x2	= tmat<float, 2, 2>;
	using dmat2x2   = tmat<double, 2, 2>;
	using imat2x2   = tmat<int, 2, 2>;
	using mat2x2_sz = tmat<size_t, 2, 2>;


	using mat2x1	= tmat<float, 2, 1>;
	using dmat2x1   = tmat<double, 2, 1>;
	using imat2x1   = tmat<int, 2, 1>;
	using mat2x1_sz = tmat<size_t, 2, 1>;

	using mat1x4	= tmat<float, 1, 4>;
	using dmat1x4   = tmat<double, 1, 4>;
	using imat1x4   = tmat<int, 1, 4>;
	using mat1x4_sz = tmat<size_t, 1, 4>;

	using mat1x3	= tmat<float, 1, 3>;
	using dmat1x3   = tmat<double, 1, 3>;
	using imat1x3   = tmat<int, 1, 3>;
	using mat1x3_sz = tmat<size_t, 1, 3>;


	using mat1x2	= tmat<float, 1, 2>;
	using dmat1x2   = tmat<double, 1, 2>;
	using imat1x2   = tmat<int, 1, 2>;
	using mat1x2_sz = tmat<size_t, 1, 2>;


	using mat1x1	= tmat<float, 1, 1>;
	using dmat1x1   = tmat<double, 1, 1>;
	using imat1x1   = tmat<int, 1, 1>;
	using mat1x1_sz = tmat<size_t, 1, 1>;
} // namespace psl



#include "math/AVX2/matrix.h"
#include "math/AVX/matrix.h"
#include "math/SSE/matrix.h"
#include "math/fallback/matrix.h"