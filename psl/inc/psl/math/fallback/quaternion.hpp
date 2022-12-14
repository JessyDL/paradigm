#pragma once
#if INSTRUCTION_SET == 0
	#include "psl/math/quaternion.hpp"

namespace psl {
template <typename precision_t>
constexpr tquat<precision_t>& operator+=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept {
	owner.value[0] += other.value[0];
	owner.value[1] += other.value[1];
	owner.value[2] += other.value[2];
	owner.value[3] += other.value[3];
	return owner;
}

template <typename precision_t>
constexpr tquat<precision_t>& operator*=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept {
	tquat<precision_t> left = owner;
	owner.value[0] = left.value[0] * other.value[3] + left.value[1] * other.value[2] - left.value[2] * other.value[1] +
					 left.value[3] * other.value[0];
	owner.value[1] = -left.value[0] * other.value[2] + left.value[1] * other.value[3] + left.value[2] * other.value[0] +
					 left.value[3] * other.value[1];
	owner.value[2] = left.value[0] * other.value[1] - left.value[1] * other.value[0] + left.value[2] * other.value[3] +
					 left.value[3] * other.value[2];
	owner.value[3] = -left.value[0] * other.value[0] - left.value[1] * other.value[1] - left.value[2] * other.value[2] +
					 left.value[3] * other.value[3];
	return owner;
}


template <typename precision_t>
constexpr tquat<precision_t>& operator/=(tquat<precision_t>& owner, const tquat<precision_t>& other) {
	#ifdef MATH_DIV_ZERO_CHECK
	if(other.value[0] == 0 || other.value[1] == 0 || other.value[2] == 0 || other.value[3] == 0)
		throw std::runtime_exception("division by 0");
	#endif
	owner.value[0] /= other.value[0];
	owner.value[1] /= other.value[1];
	owner.value[2] /= other.value[2];
	owner.value[3] /= other.value[3];
	return owner;
}
template <typename precision_t>
constexpr tquat<precision_t>& operator-=(tquat<precision_t>& owner, const tquat<precision_t>& other) noexcept {
	owner.value[0] -= other.value[0];
	owner.value[1] -= other.value[1];
	owner.value[2] -= other.value[2];
	owner.value[3] -= other.value[3];
	return owner;
}
}	 // namespace psl
#endif
