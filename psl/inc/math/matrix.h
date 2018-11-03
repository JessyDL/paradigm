#pragma once
#include "vec.h"
#include <algorithm>
#include "template_utils.h"

namespace psl
{
	template<typename precision_t, size_t dimension_x, size_t dimension_y>
	struct tmat
	{
	private:
		constexpr void fold(size_t& x, size_t& y, const precision_t& arg) noexcept
		{
			value[y][x] = arg;
			++x;
			x = x % dimension_x;
			if(x == 0)
				++y;
		}
	public:
		constexpr tmat() noexcept = default;
		template<typename... Args, typename = typename std::enable_if<sizeof...(Args) == dimension_x * dimension_y>::type>
		constexpr tmat(Args&&... args) noexcept
		{
			size_t y = 0u;
			size_t x = 0u;
			((fold(x,y, static_cast<precision_t>(args))), ...);
		};

		constexpr tmat(const precision_t& value) noexcept : 
			value(utility::templates::make_array<dimension_y, tvec<precision_t, dimension_x>>(tvec<precision_t, dimension_x>{value}))
		{ 
		};

		constexpr tvec<precision_t, dimension_x>& operator[](size_t index) noexcept
		{
			return value[index];
		}
		constexpr const tvec<precision_t, dimension_x>& operator[](size_t index) const noexcept
		{
			return value[index];
		}

		template<size_t index>
		constexpr tvec<precision_t, dimension_x>& at() noexcept
		{
			static_assert(index < dimension_y, "out of range");
			return value.at(index);
		}
		template<size_t index>
		constexpr const tvec<precision_t, dimension_x>& at() const noexcept
		{
			static_assert(index < dimension_y, "out of range");
			return value.at(index);
		}

		template<size_t index_x, size_t index_y>
		constexpr precision_t& at() noexcept
		{
			static_assert(index_x < dimension_y && index_y < dimension_x, "out of range");
			return value.at(index_x).at<index_y>();
		}
		template<size_t index_x, size_t index_y>
		constexpr const precision_t& at() const noexcept
		{
			static_assert(index_x < dimension_y && index_y < dimension_x, "out of range");
			return value.at(index_x).at<index_y>();
		}


		std::array<tvec<precision_t, dimension_x>, dimension_y> value;
	};

	using mat4x4 = tmat<float,4,4>;
	using dmat4x4 = tmat<double,4,4>;
	using imat4x4 = tmat<int,4,4>;
	using mat4x4_sz = tmat<size_t,4,4>;

	using mat4x3 = tmat<float,4,3>;
	using dmat4x3 = tmat<double,4,3>;
	using imat4x3 = tmat<int,4,3>;
	using mat4x3_sz = tmat<size_t,4,3>;


	using mat4x2 = tmat<float,4,2>;
	using dmat4x2 = tmat<double,4,2>;
	using imat4x2 = tmat<int,4,2>;
	using mat4x2_sz = tmat<size_t,4,2>;


	using mat4x1 = tmat<float,4,1>;
	using dmat4x1 = tmat<double,4,1>;
	using imat4x1 = tmat<int,4,1>;
	using mat4x1_sz = tmat<size_t,4,1>;


	using mat3x4 = tmat<float,3,4>;
	using dmat3x4 = tmat<double,3,4>;
	using imat3x4 = tmat<int,3,4>;
	using mat3x4_sz = tmat<size_t,3,4>;

	using mat3x3 = tmat<float,3,3>;
	using dmat3x3 = tmat<double,3,3>;
	using imat3x3 = tmat<int,3,3>;
	using mat3x3_sz = tmat<size_t,3,3>;


	using mat3x2 = tmat<float,3,2>;
	using dmat3x2 = tmat<double,3,2>;
	using imat3x2 = tmat<int,3,2>;
	using mat3x2_sz = tmat<size_t,3,2>;


	using mat3x1 = tmat<float,3,1>;
	using dmat3x1 = tmat<double,3,1>;
	using imat3x1 = tmat<int,3,1>;
	using mat3x1_sz = tmat<size_t,3,1>;

	using mat2x4 = tmat<float,2,4>;
	using dmat2x4 = tmat<double,2,4>;
	using imat2x4 = tmat<int,2,4>;
	using mat2x4_sz = tmat<size_t,2,4>;

	using mat2x3 = tmat<float,2,3>;
	using dmat2x3 = tmat<double,2,3>;
	using imat2x3 = tmat<int,2,3>;
	using mat2x3_sz = tmat<size_t,2,3>;


	using mat2x2 = tmat<float,2,2>;
	using dmat2x2 = tmat<double,2,2>;
	using imat2x2 = tmat<int,2,2>;
	using mat2x2_sz = tmat<size_t,2,2>;


	using mat2x1 = tmat<float,2,1>;
	using dmat2x1 = tmat<double,2,1>;
	using imat2x1 = tmat<int,2,1>;
	using mat2x1_sz = tmat<size_t,2,1>;
	
	using mat1x4 = tmat<float,1,4>;
	using dmat1x4 = tmat<double,1,4>;
	using imat1x4 = tmat<int,1,4>;
	using mat1x4_sz = tmat<size_t,1,4>;

	using mat1x3 = tmat<float,1,3>;
	using dmat1x3 = tmat<double,1,3>;
	using imat1x3 = tmat<int,1,3>;
	using mat1x3_sz = tmat<size_t,1,3>;


	using mat1x2 = tmat<float,1,2>;
	using dmat1x2 = tmat<double,1,2>;
	using imat1x2 = tmat<int,1,2>;
	using mat1x2_sz = tmat<size_t,1,2>;


	using mat1x1 = tmat<float,1,1>;
	using dmat1x1 = tmat<double,1,1>;
	using imat1x1 = tmat<int,1,1>;
	using mat1x1_sz = tmat<size_t,1,1>;
}