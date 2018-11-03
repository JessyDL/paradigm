#pragma once
#include "vec.h"

namespace psl
{
	template<typename precision_t, size_t dimension_x, size_t dimension_y>
	class tmat
	{

		std::array<precision_t, dimension_x * dimension_y> value;
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


	/*using matNxO = tmat<float,N,O>;
	using dmatNxO = tmat<double,N,O>;
	using imatNxO = tmat<int,N,O>;
	using matNxO_sz = tmat<size_t,N,O>;*/
}