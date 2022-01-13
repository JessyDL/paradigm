#include "psl/math/math.hpp"
#include "stdafx_tests.h"

using namespace psl;
using namespace math;
using namespace std;	// NOTICE: needed for GCC's bug related to: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79700

#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

namespace psl
{
	template <typename T>
	std::string to_string(const tvec<T, 4>& v) noexcept
	{
		return std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " + std::to_string(v[2]) + ", " +
			   std::to_string(v[3]);
	}

	template <typename T>
	std::string to_string(const tvec<T, 3>& v) noexcept
	{
		return std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " + std::to_string(v[2]);
	}

	template <typename T>
	std::string to_string(const tquat<T>& v) noexcept
	{
		return std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " + std::to_string(v[2]) + ", " +
			   std::to_string(v[3]);
	}
}	 // namespace psl

using namespace litmus;
namespace
{
	auto m = suite<"mathematics">() = []() {
		tvec<float, 4> v1 {1, 2, 5, 0};
		tvec<float, 4> v2 {5, 3, 2, 3};

		tvec<float, 3> u1 {1, 0, 3};
		tvec<float, 3> u2 {1, 5, 7};

		quat q1 {1, 5, 3, 0};
		quat q2 {1, 5, 3, 5};

		auto operation = []<typename Op>(auto v1, auto v2) {
			auto v3 = Op {}(v1, v2);
			require(v3) ==
			  tvec<float, 4> {Op {}(v1[0], v2[0]), Op {}(v1[1], v2[1]), Op {}(v1[2], v2[2]), Op {}(v1[3], v2[3])};

			v1 = Op {}(v1, v2);
			require(v3) == v1;
			require(v3) != v2;
		};

		section<"vectors::equality">() = [&] {
			require(v1) == v1;
			require(v1) != v2;
			auto v3 = v1;
			require(v1) == v3;
			require(v2) != v3;
			v3 = v2;
			require(v1) != v3;
			require(v2) == v3;
		};
		section<"quaternions::equality">() = [&] {
			require(q1) == q1;
			require(q1) != q2;
			auto q3 = q1;
			require(q1) == q3;
			require(q2) != q3;
			q3 = q2;
			require(q1) != q3;
			require(q2) == q3;
		};
		section<"vectors::addition">() = [&] {
			auto v3 = v1 + v2;
			require(v3) == tvec<float, 4> {v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2], v1[3] + v2[3]};

			v1 += v2;
			require(v3) == v1;
			require(v3) != v2;
		};
		section<"quaternions::addition">() = [&] {
			auto q3 = q1 + q2;
			require(q3) == quat {q1[0] + q2[0], q1[1] + q2[1], q1[2] + q2[2], q1[3] + q2[3]};

			q1 += q2;
			require(q3) == q1;
			require(q3) != q2;
		};
		section<"vectors::subtraction">() = [&] {
			auto v3 = v1 - v2;
			require(v3) == tvec<float, 4> {v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2], v1[3] - v2[3]};

			v1 -= v2;
			require(v3) == v1;
			require(v3) != v2;
		};
		section<"quaternions::subtraction">() = [&] {
			auto q3 = q1 - q2;
			require(q3) == quat {q1[0] - q2[0], q1[1] - q2[1], q1[2] - q2[2], q1[3] - q2[3]};

			q1 -= q2;
			require(q3) == q1;
			require(q3) != q2;
		};
		section<"vectors::multiplication">() = [&] {
			auto v3 = v1 * v2;
			require(v3) == tvec<float, 4> {v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2], v1[3] * v2[3]};

			v1 *= v2;
			require(v3) == v1;
			require(v3) != v2;
		};
		// todo
		// section<"quaternions::subtraction">() = [&] {};
		section<"vectors::division">() = [&] {
			auto v3 = v1 / v2;
			require(v3) == tvec<float, 4> {v1[0] / v2[0], v1[1] / v2[1], v1[2] / v2[2], v1[3] / v2[3]};

			v1 /= v2;
			require(v3) == v1;
			require(v3) != v2;
		};
		section<"quaternions::division">() = [&] {
			auto q3 = q1 / q2;
			require(q3) == quat {q1[0] / q2[0], q1[1] / q2[1], q1[2] / q2[2], q1[3] / q2[3]};

			q1 /= q2;
			require(q3) == q1;
			require(q3) != q2;
		};
		section<"vectors::dot">() = [&] {
			require(dot(tvec<float, 3> {1, 0, 3}, tvec<float, 3> {1, 5, 7})) == 22.0f;
			require(dot(v1, v2)) == 21.0f;
			require(dot(tvec<double, 4> {1, 2, 5, 0}, tvec<double, 4> {5, 3, 2, 3})) == 21.0;
		};
		section<"quaternions::dot">() = [&] { require(dot(dquat {1, 2, 5, 0}, dquat {5, 3, 2, 3})) == 21.0; };
		section<"vectors::cross">()	  = [&] {
			  require(cross(tvec<float, 3> {1, 0, 3}, tvec<float, 3> {1, 5, 7})) == tvec<float, 3> {-15, -4, 5};
			  require(cross(tvec<double, 3> {1, 2, 5}, tvec<double, 3> {5, 3, 2})) == tvec<double, 3> {-11, 23, -7};
			  require(dot(cross(tvec<float, 3> {1, 0, 3}, tvec<float, 3> {1, 5, 7}), tvec<float, 3> {1, 5, 7})) == 0;
		};
		section<"vectors::magnitude">() = [&] {
			require(magnitude(tvec<float, 3> {1, 0, 3})) == sqrtf(10.0f);
			require(square_magnitude(tvec<float, 3> {1, 0, 3})) == 10.0f;
		};
		section<"vectors::sqrt">() = [&] {
			require(sqrt(tvec<float, 3> {1, 0, 3})) == tvec<float, 3> {sqrtf(1.0f), 0.0f, sqrtf(3.0f)};
		};
		section<"vectors::pow">() = [&] {
			float pow_v = 2.0f;
			require(pow(tvec<float, 3> {1, 0, 3})) == tvec<float, 3> {powf(1, pow_v), 0, powf(3, pow_v)};
			pow_v = 5.3541f;
			require(pow(tvec<float, 3> {1, 0, 3}, pow_v)) == tvec<float, 3> {powf(1, pow_v), 0, powf(3, pow_v)};
		};
		section<"vectors::normalize">() = [&] {
			auto mag = magnitude(tvec<float, 3> {1, 0, 3});
			require(normalize(tvec<float, 3> {1, 0, 3})) == tvec<float, 3> {1.0f / mag, 0, 3.0f / mag};
			require(normalize(tvec<float, 3> {8, 0, 0})) == tvec<float, 3> {1, 0, 0};
		};
		section<"quaternions::angle_axis">() = [&] {
			require(angle_axis(85.0, dvec3(30, 15, 8))) ==
			  dquat {-29.882595093587820, -14.941297546793910, -7.9686920249567521, 0.088383699305805544};
			require(angle_axis(.5f, vec3(783, 178, 62))) == quat {193.717300f, 44.0379066f, 15.3390455f, 0.968912423f};
		};
		section<"quaternions::to/from_euler">() = [&] {
			auto vec  = dvec3 {5, 10, 90};
			auto quat = from_euler(vec);
			auto vec2 = to_euler(quat);

			// due to precision errors we will round
			vec2[0] = round(vec2[0]);
			vec2[1] = round(vec2[1]);
			vec2[2] = round(vec2[2]);
			require(vec) == vec2;
		};
		section<"quaternions::conjugate">() = [&] {
			dquat dq {15, 3, 5, 8};
			require(conjugate(dq)) == dquat {-15, -3, -5, 8};
		};
		section<"quaternions::inverse">() = [&] {
			dquat dq {15, 3, 5, 8};
			double mag = dot(dq, dq);
			require(inverse(dq)) == dquat {-15.0 / mag, -3.0 / mag, -5.0 / mag, 8.0 / mag};
		};
	};
}