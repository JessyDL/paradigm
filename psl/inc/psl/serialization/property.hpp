#pragma once
#include <type_traits>

#include "psl/concepts.hpp"
#include "psl/details/fixed_astring.hpp"

#include "psl/crc32.hpp"


namespace psl::serialization {
namespace details {
	/// @brief
	/// @tparam T
	/// \todo add overloads for anonymous_property interacting with other anonymous_property's
	template <typename T>
	class anonymous_property {
	  public:
		using value_type = T;

#pragma region constructors
		template <typename... Args>
		constexpr anonymous_property(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
			: value(std::forward<Args>(args)...) {};

		constexpr anonymous_property(const anonymous_property& rhs) noexcept(
		  std::is_nothrow_copy_constructible_v<value_type>) requires(std::is_copy_constructible_v<value_type>)
			: value(rhs.value) {}

		constexpr anonymous_property(anonymous_property&& rhs) noexcept(
		  std::is_nothrow_move_constructible_v<value_type>) requires(std::is_move_constructible_v<value_type>)
			: value(rhs.value) {}

		constexpr auto operator=(const anonymous_property& rhs) noexcept(std::is_nothrow_copy_assignable_v<value_type>)
		  -> anonymous_property& requires(std::is_copy_assignable_v<value_type>) {
			if(this != std::addressof(rhs)) {
				value = rhs.value;
			}
			return *this;
		}

		constexpr auto operator=(anonymous_property&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>)
		  -> anonymous_property& requires(std::is_move_assignable_v<value_type>) {
			if(this != std::addressof(rhs)) {
				value = std::move(rhs.value);
			}
			return *this;
		}
#pragma endregion
#pragma region operators
#pragma region assignment
		template <typename rhs_t>
		constexpr force_inline auto operator=(const rhs_t& rhs) noexcept(
		  concepts::IsAssignableNothrow<value_type, rhs_t>) requires(concepts::IsAssignable<value_type, rhs_t>) {
			return value = rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator+=(const rhs_t& rhs) noexcept(concepts::IsAdditionAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsAdditionAssignable<value_type, rhs_t>) {
			return value += rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator-=(const rhs_t& rhs) noexcept(concepts::IsSubtractionAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsSubtractionAssignable<value_type, rhs_t>) {
			return value -= rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator*=(const rhs_t& rhs) noexcept(concepts::IsMultiplicationAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsMultiplicationAssignable<value_type, rhs_t>) {
			return value *= rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator/=(const rhs_t& rhs) noexcept(concepts::IsDivisionAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsDivisionAssignable<value_type, rhs_t>) {
			return value /= rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator%=(const rhs_t& rhs) noexcept(concepts::IsModuloAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsModuloAssignable<value_type, rhs_t>) {
			return value %= rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator&=(const rhs_t& rhs) noexcept(concepts::IsBitAndAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsBitAndAssignable<value_type, rhs_t>) {
			return value &= rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator|=(const rhs_t& rhs) noexcept(concepts::IsBitOrAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsBitOrAssignable<value_type, rhs_t>) {
			return value |= rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator^=(const rhs_t& rhs) noexcept(concepts::IsBitXorAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsBitXorAssignable<value_type, rhs_t>) {
			return value ^= rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator<<=(const rhs_t& rhs) noexcept(concepts::IsShiftLeftAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsShiftLeftAssignable<value_type, rhs_t>) {
			return value <<= rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto
		operator>>=(const rhs_t& rhs) noexcept(concepts::IsShiftRightAssignableNothrow<value_type, rhs_t>) requires(
		  concepts::IsShiftRightAssignable<value_type, rhs_t>) {
			return value >>= rhs;
		}
#pragma endregion
#pragma region incdec
		constexpr force_inline auto operator++() noexcept(concepts::IsIncrementablePreNothrow<value_type>) requires(
		  concepts::IsIncrementablePre<value_type>) {
			return ++value;
		}
		constexpr force_inline auto operator++(int) noexcept(concepts::IsIncrementablePostNothrow<value_type>) requires(
		  concepts::IsIncrementablePost<value_type>) {
			return value++;
		}
		constexpr force_inline auto operator--() noexcept(concepts::IsDecrementablePreNothrow<value_type>) requires(
		  concepts::IsDecrementablePre<value_type>) {
			return --value;
		}
		constexpr force_inline auto operator--(int) noexcept(concepts::IsDecrementablePostNothrow<value_type>) requires(
		  concepts::IsDecrementablePost<value_type>) {
			return value--;
		}
#pragma endregion
#pragma region arithmetic
		constexpr force_inline auto operator+() const
		  noexcept(concepts::HasUnaryAddNothrow<value_type>) requires(concepts::HasUnaryAdd<value_type>) {
			return +value;
		}

		constexpr force_inline auto operator-() const
		  noexcept(concepts::HasUnarySubtractNothrow<value_type>) requires(concepts::HasUnarySubtract<value_type>) {
			return -value;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator+(const rhs_t& rhs) const
		  noexcept(concepts::HasAdditionNothrow<value_type, rhs_t>) requires(concepts::HasAddition<value_type, rhs_t>) {
			return value + rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator-(const rhs_t& rhs) const noexcept(
		  concepts::HasSubtractionNothrow<value_type, rhs_t>) requires(concepts::HasSubtraction<value_type, rhs_t>) {
			return value - rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator*(const rhs_t& rhs) const
		  noexcept(concepts::HasMultiplicationNothrow<value_type, rhs_t>) requires(
			concepts::HasMultiplication<value_type, rhs_t>) {
			return value * rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator/(const rhs_t& rhs) const
		  noexcept(concepts::HasDivisionNothrow<value_type, rhs_t>) requires(concepts::HasDivision<value_type, rhs_t>) {
			return value / rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator%(const rhs_t& rhs) const
		  noexcept(concepts::HasModuloNothrow<value_type, rhs_t>) requires(concepts::HasModulo<value_type, rhs_t>) {
			return value % rhs;
		}

		constexpr force_inline auto operator-() const
		  noexcept(concepts::HasBitNotNothrow<value_type>) requires(concepts::HasBitNot<value_type>) {
			return ~value;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator&(const rhs_t& rhs) const
		  noexcept(concepts::HasBitAndNothrow<value_type, rhs_t>) requires(concepts::HasBitAnd<value_type, rhs_t>) {
			return value & rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator|(const rhs_t& rhs) const
		  noexcept(concepts::HasBitOrNothrow<value_type, rhs_t>) requires(concepts::HasBitOr<value_type, rhs_t>) {
			return value | rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator^(const rhs_t& rhs) const
		  noexcept(concepts::HasBitXorNothrow<value_type, rhs_t>) requires(concepts::HasBitXor<value_type, rhs_t>) {
			return value ^ rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator<<(const rhs_t& rhs) const noexcept(
		  concepts::HasBitLeftShiftNothrow<value_type, rhs_t>) requires(concepts::HasBitLeftShift<value_type, rhs_t>) {
			return value << rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator>>(const rhs_t& rhs) const
		  noexcept(concepts::HasBitRightShiftNothrow<value_type, rhs_t>) requires(
			concepts::HasBitRightShift<value_type, rhs_t>) {
			return value >> rhs;
		}
#pragma endregion
#pragma region logical
		constexpr force_inline auto operator!() const noexcept(concepts::HasLogicalNotNothrow<value_type>)
		  -> bool requires(concepts::HasLogicalNot<value_type>) {
			return !value;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator&&(const rhs_t& rhs) const
		  noexcept(concepts::HasLogicalAndNothrow<value_type, rhs_t>)
			-> bool requires(concepts::HasLogicalAnd<value_type, rhs_t>) {
			return value && rhs;
		}

		template <typename rhs_t>
		constexpr force_inline auto operator||(const rhs_t& rhs) const
		  noexcept(concepts::HasLogicalOrNothrow<value_type, rhs_t>)
			-> bool requires(concepts::HasLogicalOr<value_type, rhs_t>) {
			return value || rhs;
		}
#pragma endregion
#pragma region comparison
		/*
			Comparison operators
		*/
		template <typename rhs_t>
		constexpr force_inline auto operator==(const rhs_t& rhs) noexcept(
		  concepts::HasEqualityNothrow<value_type, rhs_t>) requires(concepts::HasEquality<value_type, rhs_t>) {
			return value == rhs;
		}
		template <typename rhs_t>
		constexpr force_inline auto operator!=(const rhs_t& rhs) noexcept(
		  concepts::HasInequalityNothrow<value_type, rhs_t>) requires(concepts::HasInequality<value_type, rhs_t>) {
			return value != rhs;
		}
		template <typename rhs_t>
		constexpr force_inline auto operator<(const rhs_t& rhs) noexcept(
		  concepts::HasLessThanNothrow<value_type, rhs_t>) requires(concepts::HasLessThan<value_type, rhs_t>) {
			return value < rhs;
		}
		template <typename rhs_t>
		constexpr force_inline auto operator>(const rhs_t& rhs) noexcept(
		  concepts::HasGreaterThanNothrow<value_type, rhs_t>) requires(concepts::HasGreaterThan<value_type, rhs_t>) {
			return value > rhs;
		}
		template <typename rhs_t>
		constexpr force_inline auto
		operator<=(const rhs_t& rhs) noexcept(concepts::HasLessEqualThanNothrow<value_type, rhs_t>) requires(
		  concepts::HasLessEqualThan<value_type, rhs_t>) {
			return value <= rhs;
		}
		template <typename rhs_t>
		constexpr force_inline auto
		operator>=(const rhs_t& rhs) noexcept(concepts::HasGreaterEqualThanNothrow<value_type, rhs_t>) requires(
		  concepts::HasGreaterEqualThan<value_type, rhs_t>) {
			return value >= rhs;
		}
#pragma endregion
#pragma region member_access

		template <typename index_t>
		constexpr force_inline auto operator[](const index_t& index) const
		  noexcept(concepts::HasOperatorSubscriptNothrow<value_type, index_t>) requires(
			concepts::HasOperatorSubscript<value_type, index_t>) {
			return value[index];
		}

		constexpr force_inline auto operator->() -> value_type* {
			return std::addressof(value);
		}
		constexpr force_inline auto operator->() const -> const value_type* {
			return std::addressof(value);
		}

#pragma endregion
#pragma region misc
		template <typename... Args>
		constexpr force_inline auto operator()(Args&&... args) const
		  noexcept(concepts::HasOperatorInvokeNothrow<value_type, Args...>) requires(
			concepts::HasOperatorInvoke<value_type, Args...>) {
			return value(std::forward<Args>(args)...);
		}

		template <typename Y>
		requires(std::is_convertible_v<value_type, Y>) constexpr force_inline operator Y() const
		  noexcept(std::is_nothrow_convertible_v<value_type, Y>) {
			return static_cast<Y>(value);
		}
#pragma endregion
#pragma endregion

		operator const value_type&() const noexcept {
			return value;
		}
		operator value_type&() noexcept {
			return value;
		}

		value_type value {};
	};
}	 // namespace details


template <psl::details::fixed_astring Name, typename T>
class property final : public details::anonymous_property<T> {
	using base_t = details::anonymous_property<T>;

  public:
	using value_type = typename base_t::value_type;
	static constexpr std::string_view name() noexcept { return Name; }
	static constexpr uint64_t ID {utility::crc64<Name.size()>(Name)};

	using base_t::operator==;
	using base_t::operator!=;
	using base_t::operator<=;
	using base_t::operator>=;
	using base_t::operator<;
	using base_t::operator>;
	using details::anonymous_property<T>::anonymous_property;
};
}	 // namespace psl::serialization
