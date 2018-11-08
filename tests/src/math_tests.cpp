#include "stdafx_tests.h"
#include "math/math.hpp"

using namespace psl;
using namespace math;

TEST_CASE("vec math", "[MATH]")
{
	tvec<float, 4> v1{ 1,2,5,0  };
	tvec<float, 4> v2{ 5,3,2,3 };

	tvec<float, 3> u1{ 1,0,3  };
	tvec<float, 3> u2{ 1,5,7 };

	SECTION("equality")
	{
		REQUIRE(v1 == v1);
		REQUIRE(v1 != v2);
		auto v3 = v1;
		REQUIRE(v1 == v3);
	}
	SECTION("addition")
	{
		auto v3 = v1 + v2;

		REQUIRE(v3 == tvec<float, 4>{v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2], v1[3] + v2[3]});

		v1 += v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}
	SECTION("subtraction")
	{
		auto v3 = v1 - v2;

		REQUIRE(v3 == tvec<float, 4>{v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2], v1[3] - v2[3]});

		v1 -= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}

	SECTION("multiplication")
	{
		auto v3 = v1 * v2;

		REQUIRE(v3 == tvec<float, 4>{v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2], v1[3] * v2[3]});

		v1 *= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}

	SECTION("division")
	{
		auto v3 = v1 / v2;

		REQUIRE(v3 == tvec<float, 4>{v1[0] / v2[0], v1[1] / v2[1], v1[2] / v2[2], v1[3] / v2[3]});

		v1 /= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}

	SECTION("dot")
	{
		REQUIRE(dot(tvec<float, 3>{ 1,0,3  }, tvec<float, 3>{ 1,5,7 }) == 22.0f);
		REQUIRE(dot(v1,v2) == 21.0f);
		REQUIRE(dot(tvec<double, 4>{ 1,2,5,0  },tvec<double, 4>{ 5,3,2,3 }) == 21.0);
	}

	SECTION("cross")
	{
		REQUIRE(cross(tvec<float, 3>{ 1,0,3  }, tvec<float, 3>{ 1,5,7 }) == tvec<float, 3>{ -15,-4, 5 });
		REQUIRE(cross(tvec<double, 3>{ 1,2,5  }, tvec<double, 3>{ 5,3,2 }) == tvec<double, 3>{ -11,23, -7 });
	}

	SECTION("magnitude")
	{
		REQUIRE(magnitude(tvec<float, 3>{ 1, 0, 3  }) == std::sqrtf(10.0f));
		REQUIRE(square_magnitude(tvec<float, 3>{ 1, 0, 3  }) == 10.0f);
	}
	SECTION("sqrt")
	{
		REQUIRE(sqrt(tvec<float, 3>{ 1, 0, 3  }) == tvec<float, 3>{ std::sqrtf(1.0f), 0.0f, std::sqrtf(3.0f)  });
	}
	SECTION("pow")
	{
		float pow_v = 2.0f;
		REQUIRE(pow(tvec<float, 3>{ 1, 0, 3  }) == tvec<float, 3>{ std::powf(1,pow_v), 0, std::powf(3,pow_v)  });
		pow_v = 5.3541f;
		REQUIRE(pow(tvec<float, 3>{ 1, 0, 3  }, pow_v) == tvec<float, 3>{ std::powf(1,pow_v), 0, std::powf(3,pow_v)  });
	}
}