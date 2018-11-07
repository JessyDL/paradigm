#include "stdafx_tests.h"
#include "math/math.hpp"

TEST_CASE("vec math", "[MATH]")
{
	psl::tvec<float, 4> v1{ 1,2,5,0  };
	psl::tvec<float, 4> v2{ 5,3,2,3 };

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

		REQUIRE(v3 == psl::tvec<float, 4>{v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2], v1[3] + v2[3]});

		v1 += v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}
	SECTION("subtraction")
	{
		auto v3 = v1 - v2;

		REQUIRE(v3 == psl::tvec<float, 4>{v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2], v1[3] - v2[3]});

		v1 -= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}

	SECTION("multiplication")
	{
		auto v3 = v1 * v2;

		REQUIRE(v3 == psl::tvec<float, 4>{v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2], v1[3] * v2[3]});

		v1 *= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}

	SECTION("division")
	{
		auto v3 = v1 / v2;

		REQUIRE(v3 == psl::tvec<float, 4>{v1[0] / v2[0], v1[1] / v2[1], v1[2] / v2[2], v1[3] / v2[3]});

		v1 /= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}
}