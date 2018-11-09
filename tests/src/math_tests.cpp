#include "stdafx_tests.h"
#include "math/math.hpp"

using namespace psl;
using namespace math;

TEST_CASE("mathematics", "[MATH]")
{
	tvec<float, 4> v1{1, 2, 5, 0};
	tvec<float, 4> v2{5, 3, 2, 3};

	tvec<float, 3> u1{1, 0, 3};
	tvec<float, 3> u2{1, 5, 7};

	SECTION("vectors::equality")
	{
		REQUIRE(v1 == v1);
		REQUIRE(v1 != v2);
		auto v3 = v1;
		REQUIRE(v1 == v3);
	}

	SECTION("quaternions::equality")
	{
		quat q1{ 1,5,3,0 };
		quat q2{ 1,5,3,5 };
		quat q3 = q1;
		REQUIRE(q1 == q1);
		REQUIRE(q1 != q2);
		REQUIRE(q1 == q3);
	}
	SECTION("vectors::addition")
	{
		auto v3 = v1 + v2;

		REQUIRE(v3 == tvec<float, 4>{v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2], v1[3] + v2[3]});

		v1 += v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}
	SECTION("quaternions::addition")
	{
		quat q1{ 1,5,3,0 };
		quat q2{ 1,5,3,5 };
		auto q3 = q1 + q2;

		REQUIRE(q3 == quat{q1[0] + q2[0], q1[1] + q2[1], q1[2] + q2[2], q1[3] + q2[3]});

		q1 += q2;
		REQUIRE(q3 == q1);
		REQUIRE(q3 != q2);
	}
	SECTION("vectors::subtraction")
	{
		auto v3 = v1 - v2;

		REQUIRE(v3 == tvec<float, 4>{v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2], v1[3] - v2[3]});

		v1 -= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}
	SECTION("quaternions::subtraction")
	{
		quat q1{ 1,5,3,0 };
		quat q2{ 1,5,3,5 };
		auto q3 = q1 - q2;

		REQUIRE(q3 == quat{q1[0] - q2[0], q1[1] - q2[1], q1[2] - q2[2], q1[3] - q2[3]});

		q1 -= q2;
		REQUIRE(q3 == q1);
		REQUIRE(q3 != q2);
	}

	SECTION("vectors::multiplication")
	{
		auto v3 = v1 * v2;

		REQUIRE(v3 == tvec<float, 4>{v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2], v1[3] * v2[3]});

		v1 *= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}

	SECTION("quaternions::multiplication")
	{
		// todo
	}

	SECTION("vectors::division")
	{
		auto v3 = v1 / v2;

		REQUIRE(v3 == tvec<float, 4>{v1[0] / v2[0], v1[1] / v2[1], v1[2] / v2[2], v1[3] / v2[3]});

		v1 /= v2;
		REQUIRE(v3 == v1);
		REQUIRE(v3 != v2);
	}
	SECTION("quaternions::division")
	{
		quat q1{ 1,5,3,0 };
		quat q2{ 1,5,3,5 };
		auto q3 = q1 / q2;

		REQUIRE(q3 == quat{q1[0] / q2[0], q1[1] / q2[1], q1[2] / q2[2], q1[3] / q2[3]});

		q1 /= q2;
		REQUIRE(q3 == q1);
		REQUIRE(q3 != q2);
	}

	SECTION("vectors::dot")
	{
		REQUIRE(dot(tvec<float, 3>{1, 0, 3}, tvec<float, 3>{1, 5, 7}) == 22.0f);
		REQUIRE(dot(v1, v2) == 21.0f);
		REQUIRE(dot(tvec<double, 4>{1, 2, 5, 0}, tvec<double, 4>{5, 3, 2, 3}) == 21.0);
	}
	SECTION("quaternions::dot")
	{
		REQUIRE(dot(dquat{1, 2, 5, 0}, dquat{5, 3, 2, 3}) == 21.0);
	}

	SECTION("vectors::cross")
	{
		REQUIRE(cross(tvec<float, 3>{1, 0, 3}, tvec<float, 3>{1, 5, 7}) == tvec<float, 3>{-15, -4, 5});
		REQUIRE(cross(tvec<double, 3>{1, 2, 5}, tvec<double, 3>{5, 3, 2}) == tvec<double, 3>{-11, 23, -7});
		REQUIRE(dot(cross(tvec<float, 3>{1, 0, 3}, tvec<float, 3>{1, 5, 7}), tvec<float, 3>{1, 5, 7}) == 0);
	}
	SECTION("vectors::magnitude")
	{
		REQUIRE(magnitude(tvec<float, 3>{1, 0, 3}) == std::sqrtf(10.0f));
		REQUIRE(square_magnitude(tvec<float, 3>{1, 0, 3}) == 10.0f);
	}
	SECTION("vectors::sqrt")
	{
		REQUIRE(sqrt(tvec<float, 3>{1, 0, 3}) == tvec<float, 3>{std::sqrtf(1.0f), 0.0f, std::sqrtf(3.0f)});
	}
	SECTION("vectors::pow")
	{
		float pow_v = 2.0f;
		REQUIRE(pow(tvec<float, 3>{1, 0, 3}) == tvec<float, 3>{std::powf(1, pow_v), 0, std::powf(3, pow_v)});
		pow_v = 5.3541f;
		REQUIRE(pow(tvec<float, 3>{1, 0, 3}, pow_v) == tvec<float, 3>{std::powf(1, pow_v), 0, std::powf(3, pow_v)});
	}
	SECTION("vectors::normalize")
	{
		auto mag = magnitude(tvec<float, 3>{1, 0, 3});
		REQUIRE(normalize(tvec<float, 3>{1, 0, 3}) == tvec<float, 3>{1.0f / mag, 0, 3.0f / mag});
		REQUIRE(normalize(tvec<float, 3>{8, 0, 0}) == tvec<float, 3>{1, 0, 0});
	}

	SECTION("quaternions::angle_axis")
	{
		REQUIRE(angle_axis(85.0, dvec3(30, 15, 8)) ==
				dquat{-29.882595093587820, -14.941297546793910, -7.9686920249567521, 0.088383699305805544});
		REQUIRE(angle_axis(.5f, vec3(783, 178, 62)) == quat{193.717300f, 44.0379066f, 15.3390455f, 0.968912423f});
	}

	SECTION("quaternions::to/from_euler")
	{
		auto vec = dvec3{ 5,10,90 };
		auto quat = from_euler(vec);
		auto vec2 = to_euler(quat);

		// due to precision errors we will round
		vec2[0] = std::round(vec2[0]);
		vec2[1] = std::round(vec2[1]);
		vec2[2] = std::round(vec2[2]);
		REQUIRE(vec == vec2);
	}

	SECTION("quaternions::conjugate")
	{
		dquat dq{ 15,3,5,8 };
		REQUIRE(conjugate(dq) == dquat{ -15,-3,-5,8 });
	}

	SECTION("quaternions::inverse")
	{
		dquat dq{ 15,3,5,8 };
		double mag = dot(dq, dq);
		REQUIRE(inverse(dq) == dquat{ -15.0 / mag,-3.0/ mag,-5.0/ mag, 8.0/ mag });
	}
}