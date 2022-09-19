#pragma once
#include <type_traits>

namespace psl::utility
{
	template <typename T>
	concept IsEnumClass = std::is_enum_v<T> && !std::is_convertible_v<T, std::underlying_type_t<T>>;

	enum class enum_ops_t : unsigned char
	{
		NONE	   = 0,
		BIT		   = 1 << 0,
		SHIFT	   = 1 << 1,
		ARITHMETIC = 1 << 2,
		LOGICAL	   = 1 << 3
	};

	template <IsEnumClass T>
	inline constexpr enum_ops_t enable_enum_ops = enum_ops_t::NONE;

	template <>
	inline constexpr auto enable_enum_ops<enum_ops_t> = enum_ops_t::BIT;

	template <typename T>
	concept HasEnumBitOps = IsEnumClass<T> && static_cast<enum_ops_t>(
							  static_cast<std::underlying_type_t<enum_ops_t>>(enable_enum_ops<T>) &
							  static_cast<std::underlying_type_t<enum_ops_t>>(enum_ops_t::BIT)) == enum_ops_t::BIT;
	template <typename T>
	concept HasEnumShiftOps =
	  IsEnumClass<T> &&
	  static_cast<enum_ops_t>(static_cast<std::underlying_type_t<enum_ops_t>>(enable_enum_ops<T>) &
							  static_cast<std::underlying_type_t<enum_ops_t>>(enum_ops_t::SHIFT)) == enum_ops_t::SHIFT;
	template <typename T>
	concept HasEnumArithmeticOps =
	  IsEnumClass<T> && static_cast<enum_ops_t>(
		static_cast<std::underlying_type_t<enum_ops_t>>(enable_enum_ops<T>) &
		static_cast<std::underlying_type_t<enum_ops_t>>(enum_ops_t::ARITHMETIC)) == enum_ops_t::ARITHMETIC;
	template <typename T>
	concept HasEnumLogicalOps =
	  IsEnumClass<T> && static_cast<enum_ops_t>(static_cast<std::underlying_type_t<enum_ops_t>>(enable_enum_ops<T>) &
												static_cast<std::underlying_type_t<enum_ops_t>>(enum_ops_t::LOGICAL)) ==
	  enum_ops_t::LOGICAL;

	template <HasEnumBitOps T>
	[[nodiscard]] constexpr T operator|(T const a, T const b) noexcept
	{
		using I = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<I>(a) | static_cast<I>(b));
	}

	template <HasEnumBitOps T>
	[[nodiscard]] constexpr T operator&(T const a, T const b) noexcept
	{
		using I = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<I>(a) & static_cast<I>(b));
	}

	template <HasEnumBitOps T>
	[[nodiscard]] constexpr T operator^(T const a, T const b) noexcept
	{
		using I = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<I>(a) ^ static_cast<I>(b));
	}

	template <HasEnumBitOps T>
	constexpr T& operator|=(T& a, T const b) noexcept
	{
		return a = a | b;
	}

	template <HasEnumBitOps T>
	constexpr T& operator&=(T& a, T const b) noexcept
	{
		return a = a & b;
	}

	template <HasEnumBitOps T>
	constexpr T& operator^=(T& a, T const b) noexcept
	{
		return a = a ^ b;
	}

	template <HasEnumLogicalOps T>
	constexpr bool operator&&(T const a, T const b) noexcept
	{
		using I = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<I>(a) & static_cast<I>(b)) == b;
	}

	template <HasEnumBitOps T>
	[[nodiscard]] constexpr T operator~(T const a) noexcept
	{
		using I = std::underlying_type_t<T>;
		return static_cast<T>(~static_cast<I>(a));
	}

	template <HasEnumShiftOps T>
	[[nodiscard]] constexpr T operator<<(T const a, std::size_t pos) noexcept
	{
		using I = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<I>(a) << pos);
	}

	template <HasEnumShiftOps T>
	[[nodiscard]] constexpr T operator>>(T const a, std::size_t pos) noexcept
	{
		using I = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<I>(a) >> pos);
	}

	template <HasEnumShiftOps T>
	constexpr T& operator<<=(T& a, std::size_t pos) noexcept
	{
		return a = a << pos;
	}

	template <HasEnumShiftOps T>
	constexpr T& operator>>=(T& a, std::size_t pos) noexcept
	{
		return a = a >> pos;
	}
}
