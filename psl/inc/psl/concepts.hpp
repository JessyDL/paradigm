#pragma once
#include <type_traits>

#include <concepts>

#if defined(_MSC_VER)
#define force_inline __inline
#else
#define force_inline __attribute__((always_inline))
#endif

/// \brief Utility concepts
/// 
namespace psl::concepts
{
	template <typename Fn, typename... Args>
	concept IsInvocableNothrow = std::is_nothrow_invocable<Fn, Args...>::value;

	template <typename Fn, typename... Args>
	concept IsInvocable = std::is_invocable_v<Fn, Args...>;
#pragma region operators
#pragma region assignment
	template <typename T, typename Y>
	concept IsAssignable = std::is_assignable_v<T, Y>;
	template <typename T, typename Y>
	concept IsAssignableNothrow = std::is_nothrow_assignable_v<T, Y>;

	template <typename T, typename Y>
	concept IsAdditionAssignable = requires(T lhs, Y rhs)
	{
		lhs += rhs;
	};
	template <typename T, typename Y>
	concept IsAdditionAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs += rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsSubtractionAssignable = requires(T lhs, Y rhs)
	{
		lhs -= rhs;
	};
	template <typename T, typename Y>
	concept IsSubtractionAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs -= rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsMultiplicationAssignable = requires(T lhs, Y rhs)
	{
		lhs *= rhs;
	};
	template <typename T, typename Y>
	concept IsMultiplicationAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs *= rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsDivisionAssignable = requires(T lhs, Y rhs)
	{
		lhs /= rhs;
	};
	template <typename T, typename Y>
	concept IsDivisionAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs /= rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsModuloAssignable = requires(T lhs, Y rhs)
	{
		lhs %= rhs;
	};
	template <typename T, typename Y>
	concept IsModuloAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs %= rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsBitAndAssignable = requires(T lhs, Y rhs)
	{
		lhs &= rhs;
	};
	template <typename T, typename Y>
	concept IsBitAndAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs &= rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsBitOrAssignable = requires(T lhs, Y rhs)
	{
		lhs |= rhs;
	};
	template <typename T, typename Y>
	concept IsBitOrAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs |= rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsBitXorAssignable = requires(T lhs, Y rhs)
	{
		lhs ^= rhs;
	};
	template <typename T, typename Y>
	concept IsBitXorAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs ^= rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsShiftLeftAssignable = requires(T lhs, Y rhs)
	{
		lhs <<= rhs;
	};
	template <typename T, typename Y>
	concept IsShiftLeftAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs <<= rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept IsShiftRightAssignable = requires(T lhs, Y rhs)
	{
		lhs >>= rhs;
	};
	template <typename T, typename Y>
	concept IsShiftRightAssignableNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs >>= rhs
		}
		noexcept;
	};
#pragma endregion

#pragma region incdec
	template <typename T>
	concept IsIncrementablePre = requires(T val)
	{
		++val;
	};
	template <typename T>
	concept IsIncrementablePreNothrow = requires(T val)
	{
		{
			++val
		}
		noexcept;
	};
	template <typename T>
	concept IsIncrementablePost = requires(T val)
	{
		val++;
	};
	template <typename T>
	concept IsIncrementablePostNothrow = requires(T val)
	{
		{
			val++
		}
		noexcept;
	};

	template <typename T>
	concept IsDecrementablePre = requires(T val)
	{
		--val;
	};
	template <typename T>
	concept IsDecrementablePreNothrow = requires(T val)
	{
		{
			--val
		}
		noexcept;
	};
	template <typename T>
	concept IsDecrementablePost = requires(T val)
	{
		val--;
	};
	template <typename T>
	concept IsDecrementablePostNothrow = requires(T val)
	{
		{
			val--
		}
		noexcept;
	};
#pragma endregion

#pragma region arithmetic
	template <typename T>
	concept HasUnaryAdd = requires(T val)
	{
		+val;
	};
	template <typename T>
	concept HasUnaryAddNothrow = requires(T val)
	{
		{
			+val
		}
		noexcept;
	};

	template <typename T>
	concept HasUnarySubtract = requires(T val)
	{
		-val;
	};
	template <typename T>
	concept HasUnarySubtractNothrow = requires(T val)
	{
		{
			-val
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept HasAddition = requires(T lhs, Y rhs)
	{
		lhs - rhs;
	};
	template <typename T, typename Y>
	concept HasAdditionNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs - rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept HasSubtraction = requires(T lhs, Y rhs)
	{
		lhs - rhs;
	};
	template <typename T, typename Y>
	concept HasSubtractionNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs - rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept HasMultiplication = requires(T lhs, Y rhs)
	{
		lhs* rhs;
	};
	template <typename T, typename Y>
	concept HasMultiplicationNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs* rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept HasDivision = requires(T lhs, Y rhs)
	{
		lhs / rhs;
	};
	template <typename T, typename Y>
	concept HasDivisionNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs / rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept HasModulo = requires(T lhs, Y rhs)
	{
		lhs % rhs;
	};
	template <typename T, typename Y>
	concept HasModuloNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs % rhs
		}
		noexcept;
	};

	template <typename T>
	concept HasBitNot = requires(T val)
	{
		~val;
	};
	template <typename T>
	concept HasBitNotNothrow = requires(T val)
	{
		{
			~val
		}
		noexcept;
	};


	template <typename T, typename Y>
	concept HasBitAnd = requires(T lhs, Y rhs)
	{
		lhs& rhs;
	};
	template <typename T, typename Y>
	concept HasBitAndNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs& rhs
		}
		noexcept;
	};


	template <typename T, typename Y>
	concept HasBitOr = requires(T lhs, Y rhs)
	{
		lhs | rhs;
	};
	template <typename T, typename Y>
	concept HasBitOrNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs | rhs
		}
		noexcept;
	};


	template <typename T, typename Y>
	concept HasBitXor = requires(T lhs, Y rhs)
	{
		lhs ^ rhs;
	};
	template <typename T, typename Y>
	concept HasBitXorNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs ^ rhs
		}
		noexcept;
	};

	template <typename T, typename Y>
	concept HasBitLeftShift = requires(T lhs, Y rhs)
	{
		lhs << rhs;
	};
	template <typename T, typename Y>
	concept HasBitLeftShiftNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs << rhs
		}
		noexcept;
	};


	template <typename T, typename Y>
	concept HasBitRightShift = requires(T lhs, Y rhs)
	{
		lhs >> rhs;
	};
	template <typename T, typename Y>
	concept HasBitRightShiftNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs >> rhs
		}
		noexcept;
	};

#pragma endregion
#pragma region logical
	template <typename T>
	concept HasLogicalNot = requires(T val)
	{
		{
			!val
		}
		->std::same_as<bool>;
	};

	template <typename T>
	concept HasLogicalNotNothrow = requires(T val)
	{
		{
			!val
		}
		noexcept->std::same_as<bool>;
	};

	template <typename T, typename Y>
	concept HasLogicalAnd = requires(T lhs, Y rhs)
	{
		{
			lhs&& rhs
		}
		->std::same_as<bool>;
	};
	template <typename T, typename Y>
	concept HasLogicalAndNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs&& rhs
		}
		noexcept->std::same_as<bool>;
	};

	template <typename T, typename Y>
	concept HasLogicalOr = requires(T lhs, Y rhs)
	{
		{
			lhs || rhs
		}
		->std::same_as<bool>;
	};
	template <typename T, typename Y>
	concept HasLogicalOrNothrow = requires(T lhs, Y rhs)
	{
		{
			lhs || rhs
		}
		noexcept->std::same_as<bool>;
	};
#pragma endregion
#pragma region comparison
	template <typename T, typename Y = T>
	concept HasEquality = IsInvocable<std::equal_to<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasEqualityNothrow = IsInvocableNothrow<std::equal_to<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasInequality = IsInvocable<std::not_equal_to<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasInequalityNothrow = IsInvocableNothrow<std::not_equal_to<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasGreaterThan = IsInvocable<std::greater<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasGreaterThanNothrow = IsInvocableNothrow<std::greater<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasGreaterEqualThan = IsInvocable<std::greater_equal<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasGreaterEqualThanNothrow = IsInvocableNothrow<std::greater_equal<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasLessThan = IsInvocable<std::less<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasLessThanNothrow = IsInvocableNothrow<std::less<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasLessEqualThan = IsInvocable<std::less_equal<>, T, Y>;

	template <typename T, typename Y = T>
	concept HasLessEqualThanNothrow = IsInvocableNothrow<std::less_equal<>, T, Y>;
#pragma endregion

#pragma region member_access

	template <typename T, typename Y>
	concept HasOperatorSubscript = requires(T val, Y index)
	{
		val[index];
	};
	template <typename T, typename Y>
	concept HasOperatorSubscriptNothrow = requires(T val, Y index)
	{
		{
			val[index]
		}
		noexcept;
	};


#pragma endregion

#pragma region misc
	template <typename T, typename... Args>
	concept HasOperatorInvoke = requires(T val, Args... args)
	{
		val(args...);
	};
	template <typename T, typename... Args>
	concept HasOperatorInvokeNothrow = requires(T val, Args... args)
	{
		{
			val(args...)
		}
		noexcept;
	};
#pragma endregion

#pragma endregion
}	 // namespace psl::concepts
