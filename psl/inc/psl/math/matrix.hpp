#pragma once
#include "psl/template_utils.hpp"
#include "vec.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>	  // std::mem*

namespace psl {
// stored as column-major internally
template <typename precision_t, size_t columns_n, size_t rows_n>
struct tmat {
	static constexpr auto column_length = rows_n;
	static constexpr auto row_length	= columns_n;

  private:
	inline constexpr auto index_of(size_t row, size_t column) const noexcept { return row * columns_n + column; }

  public:
	constexpr tmat() noexcept = default;
	template <typename... Args, typename = typename std::enable_if<sizeof...(Args) == columns_n * rows_n>::type>
	constexpr tmat(Args&&... args) noexcept : value({static_cast<precision_t>(std::forward<Args>(args))...}) {};

	constexpr tmat(const precision_t& val) noexcept : value({0}) {
		for(size_t i = 0; i < columns_n; ++i) {
			value[i * columns_n + i] = val;
		}
	};

	template <size_t columns2_n, size_t rows2_n>
	constexpr tmat(const tmat<precision_t, columns2_n, rows2_n>& val) noexcept : value({0}) {
		for(size_t i = 0; i < columns_n; ++i) {
			value[i * columns_n + i] = 1;
		}
		for(size_t i = 0; i < rows2_n; ++i) {
			for(size_t x = 0; x < columns2_n; ++x) {
				value[x + columns_n * i] = val[x + columns2_n * i];
			}
		}
	}
	constexpr precision_t& operator[](size_t index) noexcept { return value[index]; }
	constexpr const precision_t& operator[](size_t index) const noexcept { return value[index]; }


	constexpr precision_t& operator[](const size_t (&index)[2]) noexcept {
		return this->operator()(index[0], index[1]);
	}
	constexpr const precision_t& operator[](const size_t (&index)[2]) const noexcept {
		return this->operator()(index[0], index[1]);
	}

	constexpr precision_t& operator()(size_t row, size_t column) noexcept { return value[index_of(row, column)]; }
	constexpr const precision_t& operator()(size_t row, size_t column) const noexcept {
		return value[index_of(row, column)];
	}

	constexpr precision_t& operator()(size_t index) { return value[index]; }

	template <size_t row, size_t column>
	constexpr precision_t& at() noexcept {
		static_assert(row < rows_n && column < columns_n, "out of range");
		return value[index_of(row, column)];
	}
	template <size_t row, size_t column>
	constexpr const precision_t& at() const noexcept {
		static_assert(row < rows_n && column < columns_n, "out of range");
		return value[index_of(row, column)];
	}

	template <size_t index>
	constexpr tvec<precision_t, column_length> column() const noexcept {
		static_assert(index < columns_n, "out of range");
		tvec<precision_t, column_length> res {};
		std::memcpy(res.value.data(), &value[index_of(0, index)], sizeof(precision_t) * column_length);
		return res;
	}

	constexpr tvec<precision_t, column_length> column(size_t index) const noexcept {
		psl_assert(index < columns_n, "out of range {} < {}", index, columns_n);
		tvec<precision_t, column_length> res {};
		std::memcpy(res.value.data(), &value[index_of(0, index)], sizeof(precision_t) * column_length);
		return res;
	}

	template <size_t index>
	constexpr void column(const tvec<precision_t, column_length>& vector) noexcept {
		static_assert(index < columns_n, "out of range");
		std::memcpy(&value[index_of(0, index)], vector.value.data(), sizeof(precision_t) * column_length);
	}

	constexpr void column(size_t index, const tvec<precision_t, column_length>& vector) noexcept {
		psl_assert(index < columns_n, "out of range {} < {}", index, columns_n);
		std::memcpy(&value[index_of(0, index)], vector.value.data(), sizeof(precision_t) * column_length);
	}

	template <size_t index>
	constexpr tvec<precision_t, columns_n> row() const noexcept {
		static_assert(index < rows_n, "out of range");
		tvec<precision_t, row_length> res {};
		for(size_t i = 0; i < row_length; ++i) res[i] = value[index_of(index, i)];
		return res;
	}

	constexpr tvec<precision_t, columns_n> row(size_t index) const noexcept {
		psl_assert(index < rows_n, "out of range {} < {}", index, rows_n);
		tvec<precision_t, row_length> res {};
		for(size_t i = 0; i < row_length; ++i) res[i] = value[index_of(index, i)];
		return res;
	}

	template <size_t index>
	constexpr void row(const tvec<precision_t, row_length>& vector) noexcept {
		static_assert(index < rows_n, "out of range");
		for(size_t i = 0; i < row_length; ++i) value[index_of(index, i)] = vector[i];
	}

	constexpr void row(size_t index, const tvec<precision_t, row_length>& vector) noexcept {
		psl_assert(index < rows_n, "out of range {} < {}", index, rows_n);
		for(size_t i = 0; i < row_length; ++i) value[index_of(index, i)] = vector[i];
	}


	constexpr psl::tvec<precision_t, column_length>
	operator*(const tvec<precision_t, column_length>& target) const noexcept {
		psl::tvec<precision_t, column_length> result {0};
		for(size_t r = 0; r < column_length; ++r)
			for(size_t c = 0; c < columns_n; ++c) result[r] += target[c] * value[c + r * columns_n];
		return result;
	}


	std::array<precision_t, columns_n * rows_n> value;
};

using mat4x4	= tmat<float, 4, 4>;
using dmat4x4	= tmat<double, 4, 4>;
using imat4x4	= tmat<int, 4, 4>;
using mat4x4_sz = tmat<size_t, 4, 4>;

using mat4x3	= tmat<float, 4, 3>;
using dmat4x3	= tmat<double, 4, 3>;
using imat4x3	= tmat<int, 4, 3>;
using mat4x3_sz = tmat<size_t, 4, 3>;


using mat4x2	= tmat<float, 4, 2>;
using dmat4x2	= tmat<double, 4, 2>;
using imat4x2	= tmat<int, 4, 2>;
using mat4x2_sz = tmat<size_t, 4, 2>;


using mat4x1	= tmat<float, 4, 1>;
using dmat4x1	= tmat<double, 4, 1>;
using imat4x1	= tmat<int, 4, 1>;
using mat4x1_sz = tmat<size_t, 4, 1>;


using mat3x4	= tmat<float, 3, 4>;
using dmat3x4	= tmat<double, 3, 4>;
using imat3x4	= tmat<int, 3, 4>;
using mat3x4_sz = tmat<size_t, 3, 4>;

using mat3x3	= tmat<float, 3, 3>;
using dmat3x3	= tmat<double, 3, 3>;
using imat3x3	= tmat<int, 3, 3>;
using mat3x3_sz = tmat<size_t, 3, 3>;


using mat3x2	= tmat<float, 3, 2>;
using dmat3x2	= tmat<double, 3, 2>;
using imat3x2	= tmat<int, 3, 2>;
using mat3x2_sz = tmat<size_t, 3, 2>;


using mat3x1	= tmat<float, 3, 1>;
using dmat3x1	= tmat<double, 3, 1>;
using imat3x1	= tmat<int, 3, 1>;
using mat3x1_sz = tmat<size_t, 3, 1>;

using mat2x4	= tmat<float, 2, 4>;
using dmat2x4	= tmat<double, 2, 4>;
using imat2x4	= tmat<int, 2, 4>;
using mat2x4_sz = tmat<size_t, 2, 4>;

using mat2x3	= tmat<float, 2, 3>;
using dmat2x3	= tmat<double, 2, 3>;
using imat2x3	= tmat<int, 2, 3>;
using mat2x3_sz = tmat<size_t, 2, 3>;


using mat2x2	= tmat<float, 2, 2>;
using dmat2x2	= tmat<double, 2, 2>;
using imat2x2	= tmat<int, 2, 2>;
using mat2x2_sz = tmat<size_t, 2, 2>;


using mat2x1	= tmat<float, 2, 1>;
using dmat2x1	= tmat<double, 2, 1>;
using imat2x1	= tmat<int, 2, 1>;
using mat2x1_sz = tmat<size_t, 2, 1>;

using mat1x4	= tmat<float, 1, 4>;
using dmat1x4	= tmat<double, 1, 4>;
using imat1x4	= tmat<int, 1, 4>;
using mat1x4_sz = tmat<size_t, 1, 4>;

using mat1x3	= tmat<float, 1, 3>;
using dmat1x3	= tmat<double, 1, 3>;
using imat1x3	= tmat<int, 1, 3>;
using mat1x3_sz = tmat<size_t, 1, 3>;


using mat1x2	= tmat<float, 1, 2>;
using dmat1x2	= tmat<double, 1, 2>;
using imat1x2	= tmat<int, 1, 2>;
using mat1x2_sz = tmat<size_t, 1, 2>;


using mat1x1	= tmat<float, 1, 1>;
using dmat1x1	= tmat<double, 1, 1>;
using imat1x1	= tmat<int, 1, 1>;
using mat1x1_sz = tmat<size_t, 1, 1>;
}	 // namespace psl


#include "psl/math/AVX/matrix.hpp"
#include "psl/math/AVX2/matrix.hpp"
#include "psl/math/SSE/matrix.hpp"
#include "psl/math/fallback/matrix.hpp"
