#pragma once
#include "psl/assertions.h"
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace utility::templates
{
	namespace details
	{
		template <size_t first, size_t second, size_t... remainder>
		static constexpr size_t max_impl() noexcept
		{
			constexpr auto val = (first > second) ? first : second;
			if constexpr(sizeof...(remainder) > 0)
			{
				return details::max_impl<val, remainder...>();
			}
			else
				return val;
		}

		template <size_t first, size_t second, size_t... remainder>
		static constexpr size_t min_impl() noexcept
		{
			constexpr auto val = (first < second) ? first : second;
			if constexpr(sizeof...(remainder) > 0)
			{
				return details::min_impl<val, remainder...>();
			}
			else
				return val;
		}
	}	 // namespace details

	template <size_t first, size_t... remainder>
	static constexpr size_t max() noexcept
	{
		if constexpr(sizeof...(remainder) > 0)
		{
			return details::max_impl<first, remainder...>();
		}
		else
			return first;
	}

	template <size_t first, size_t... remainder>
	static constexpr size_t min() noexcept
	{
		if constexpr(sizeof...(remainder) > 0)
		{
			return details::min_impl<first, remainder...>();
		}
		else
			return first;
	}

	template <class... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};
	template <class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;

	template <size_t N, class T>
	constexpr std::array<T, N> make_array(const T& v) noexcept
	{
		std::array<T, N> ret;
		ret.fill(v);
		return ret;
	}

	template <typename T>
	struct type_container
	{
		using type = T;
	};

	template <typename T, typename Tuple>
	struct has_type;

	template <typename T>
	struct has_type<T, std::tuple<>> : std::false_type
	{};

	template <typename T, typename U, typename... Ts>
	struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>>
	{};

	template <typename T, typename... Ts>
	struct has_type<T, std::tuple<T, Ts...>> : std::true_type
	{};

	template <typename T, typename Tuple>
	using tuple_contains_type = typename has_type<T, Tuple>::type;

	// handy utility for the compilers that wish to compile static_assert(false,"") in dead code paths that should error
	// out
	template <typename T>
	struct always_false : std::false_type
	{};

	template <typename T>
	inline constexpr bool always_false_v = always_false<T>::value;

	namespace detail
	{
		template <template <class...> class Trait, class Enabler, class... Args>
		struct is_detected : std::false_type
		{};

		template <template <class...> class Trait, class... Args>
		struct is_detected<Trait, std::void_t<Trait<Args...>>, Args...> : std::true_type
		{};
	}	 // namespace detail

	template <template <class...> class Trait, class... Args>
	using is_detected = typename detail::is_detected<Trait, void, Args...>::type;

	namespace match_detail
	{
		using namespace std;

		template <int variant_index, typename VariantType, typename OneFunc>
		constexpr void match(VariantType&& variant, OneFunc func)
		{
			// assert(variant.index() == variant_index); // todo write correct assertation
			if(variant.index() != variant_index) debug_break();
			func(get<variant_index>(std::forward<VariantType>(variant)));
		}

		template <int variant_index, typename VariantType, typename FirstFunc, typename SecondFunc, typename... Rest>
		constexpr void match(VariantType&& variant, FirstFunc func1, SecondFunc func2, Rest... funcs)
		{
			if(variant.index() == variant_index)
			{
				match<variant_index>(variant, func1);	 // Call single func version
				return;
			}
			return match<variant_index + 1>(std::forward<VariantType>(variant), func2, funcs...);
		}
	}	 // namespace match_detail

	template <typename... args>
	struct all_same : public std::false_type
	{};


	template <typename T>
	struct all_same<T> : public std::true_type
	{};


	template <typename T, typename... args>
	struct all_same<T, T, args...> : public all_same<T, args...>
	{};

	template <typename VariantType, typename... Funcs>
	void match(VariantType&& variant, Funcs... funcs)
	{
		using VT = std::remove_reference_t<VariantType>;
		static_assert(sizeof...(funcs) == std::variant_size<VT>::value,
					  "Number of functions must match number of variant types");
		match_detail::match<0>(std::forward<VariantType>(variant), funcs...);
	}

	template <typename T>
	struct remove_all
	{
		typedef T type;
	};
	template <typename T>
	struct remove_all<T*>
	{
		typedef typename remove_all<T>::type type;
	};

	template <bool B, typename T, T trueval, T falseval>
	struct conditional_value :
		std::conditional<B, std::integral_constant<T, trueval>, std::integral_constant<T, falseval>>::type
	{};

	template <typename T>
	struct is_pair : public std::false_type
	{};
	template <typename T>
	struct is_associative_container
	{
		static constexpr bool value {false};
	};
	template <typename T>
	struct is_trivial_container
	{
		static constexpr bool value {false};
	};
	template <typename T>
	struct is_complex_container
	{
		static constexpr bool value {false};
	};
	template <typename T>
	struct is_container
	{
		static constexpr bool value {is_trivial_container<T>::value || is_complex_container<T>::value ||
									 is_associative_container<T>::value};
	};

	template <typename T, typename A>
	struct is_trivial_container<std::vector<T, A>>
	{
		static constexpr bool value {true};
	};

	template <typename T, typename... A>
	struct is_complex_container<std::tuple<T, A...>>
	{
		static constexpr bool value {true};
	};

	template <typename T, typename A>
	struct is_complex_container<std::unordered_set<T, A>>
	{
		static constexpr bool value {true};
	};

	template <typename T, typename A>
	struct is_associative_container<std::unordered_map<T, A>>
	{
		static constexpr bool value {true};
	};

	template <typename T, typename A>
	struct is_pair<std::pair<T, A>>
	{
		static constexpr bool value {true};
	};

	template <typename T>
	struct get_key_type
	{};

	template <typename T, typename A>
	struct get_key_type<std::unordered_map<T, A>>
	{
		using type = T;
	};

	template <typename T>
	struct get_value_type
	{};

	template <typename T, typename A>
	struct get_value_type<std::unordered_map<T, A>>
	{
		using type = A;
	};

	namespace operators
	{
		namespace details
		{
			// https://stackoverflow.com/questions/6534041/how-to-check-whether-operator-exists/6534951
			template <typename X, typename Y, typename Op>
			struct op_valid_impl
			{
				template <typename U, typename L, typename R>
				static auto test(int)
				  -> decltype(std::declval<U>()(std::declval<L>(), std::declval<R>()), void(), std::true_type());

				template <typename U, typename L, typename R>
				static auto test(...) -> std::false_type;

				using type = decltype(test<Op, X, Y>(0));
			};

			template <typename T>
			struct has_pre_increment
			{
				template <typename U, typename = decltype(++(std::declval<U&>()))>
				static long test(const U&&);
				static char test(...);

				static constexpr bool value = sizeof(test(std::declval<T>())) == sizeof(long);
			};
			template <typename T>
			struct has_post_increment
			{
				template <typename U, typename = decltype((std::declval<U&>())++)>
				static long test(const U&&);
				static char test(...);

				static constexpr bool value = sizeof(test(std::declval<T>())) == sizeof(long);
			};
			template <typename T>
			struct has_pre_decrement
			{
				template <typename U, typename = decltype(--(std::declval<U&>()))>
				static long test(const U&&);
				static char test(...);

				static constexpr bool value = sizeof(test(std::declval<T>())) == sizeof(long);
			};

			template <typename T>
			struct has_post_decrement
			{
				template <typename U, typename = decltype((std::declval<U&>())--)>
				static long test(const U&&);
				static char test(...);

				static constexpr bool value = sizeof(test(std::declval<T>())) == sizeof(long);
			};


			template <typename T>
			struct has_bool_operator
			{
				template <typename U, typename = decltype((std::declval<U&>()).operator bool())>
				static long test(const U&&);
				static char test(...);

				static constexpr bool value = sizeof(test(std::declval<T>())) == sizeof(long);
			};

			// https://stackoverflow.com/questions/31305894/how-to-check-for-the-existence-of-a-subscript-operator
			template <typename T, typename Index, typename = void>
			struct has_subscript_operator : std::false_type
			{};
			template <typename T, typename Index>
			struct has_subscript_operator<T, Index, std::void_t<decltype(std::declval<T>()[std::declval<Index>()])>> :
				std::true_type
			{};

			template <typename T, typename U, typename = void>
			struct has_arithmetic_plus : std::false_type
			{};
			template <typename T, typename U>
			struct has_arithmetic_plus<T, U, std::void_t<decltype(std::declval<T>().operator+(std::declval<U>()))>> :
				std::true_type
			{};

			template <typename T, typename U, typename = void>
			struct has_arithmetic_minus : std::false_type
			{};
			template <typename T, typename U>
			struct has_arithmetic_minus<T, U, std::void_t<decltype(std::declval<T>().operator-(std::declval<U>()))>> :
				std::true_type
			{};

			template <typename T, typename U, typename = void>
			struct has_arithmetic_multiply : std::false_type
			{};
			template <typename T, typename U>
			struct has_arithmetic_multiply<T,
										   U,
										   std::void_t<decltype(std::declval<T>().operator*(std::declval<U>()))>> :
				std::true_type
			{};

			template <typename T, typename U, typename = void>
			struct has_arithmetic_divide : std::false_type
			{};
			template <typename T, typename U>
			struct has_arithmetic_divide<T, U, std::void_t<decltype(std::declval<T>().operator/(std::declval<U>()))>> :
				std::true_type
			{};

			template <typename T, typename U, typename = void>
			struct has_arithmetic_modulus : std::false_type
			{};
			template <typename T, typename U>
			struct has_arithmetic_modulus<T, U, std::void_t<decltype(std::declval<T>().operator%(std::declval<U>()))>> :
				std::true_type
			{};


			template <typename T, typename U, typename = void>
			struct has_assignment_plus : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_plus<T, U, std::void_t<decltype(std::declval<T>().operator+=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_minus : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_minus<T, U, std::void_t<decltype(std::declval<T>().operator-=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_multiply : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_multiply<T,
										   U,
										   std::void_t<decltype(std::declval<T>().operator*=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_divide : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_divide<T, U, std::void_t<decltype(std::declval<T>().operator/=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_modulus : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_modulus<T,
										  U,
										  std::void_t<decltype(std::declval<T>().operator%=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_and : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_and<T, U, std::void_t<decltype(std::declval<T>().operator&=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_or : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_or<T, U, std::void_t<decltype(std::declval<T>().operator|=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_xor : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_xor<T, U, std::void_t<decltype(std::declval<T>().operator^=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_left_shift : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_left_shift<T,
											 U,
											 std::void_t<decltype(std::declval<T>().operator<<=(std::declval<U>()))>> :
				std::true_type
			{};
			template <typename T, typename U, typename = void>
			struct has_assignment_right_shift : std::false_type
			{};
			template <typename T, typename U>
			struct has_assignment_right_shift<T,
											  U,
											  std::void_t<decltype(std::declval<T>().operator>>=(std::declval<U>()))>> :
				std::true_type
			{};

			template <typename X, typename Y, typename Op>
			using op_valid = typename op_valid_impl<X, Y, Op>::type;


			struct left_shift
			{
				template <typename L, typename R>
				constexpr auto operator()(L&& l, R&& r) const
				  noexcept(noexcept(std::forward<L>(l) << std::forward<R>(r)))
					-> decltype(std::forward<L>(l) << std::forward<R>(r))
				{
					return std::forward<L>(l) << std::forward<R>(r);
				}
			};

			struct right_shift
			{
				template <typename L, typename R>
				constexpr auto operator()(L&& l, R&& r) const
				  noexcept(noexcept(std::forward<L>(l) >> std::forward<R>(r)))
					-> decltype(std::forward<L>(l) >> std::forward<R>(r))
				{
					return std::forward<L>(l) >> std::forward<R>(r);
				}
			};
		}	 // namespace details
		// logical operators
		template <typename X, typename Y>
		using has_logical_not = details::op_valid<X, Y, std::logical_not<>>;
		template <typename X, typename Y>
		using has_logical_and = details::op_valid<X, Y, std::logical_and<>>;
		template <typename X, typename Y>
		using has_logical_or = details::op_valid<X, Y, std::logical_or<>>;

		// relational operators
		template <typename X, typename Y>
		using has_equality = details::op_valid<X, Y, std::equal_to<>>;
		template <typename X, typename Y>
		using has_inequality = details::op_valid<X, Y, std::not_equal_to<>>;
		template <typename X, typename Y>
		using has_less_than = details::op_valid<X, Y, std::less<>>;
		template <typename X, typename Y>
		using has_less_equal = details::op_valid<X, Y, std::less_equal<>>;
		template <typename X, typename Y>
		using has_greater_than = details::op_valid<X, Y, std::greater<>>;
		template <typename X, typename Y>
		using has_greater_equal = details::op_valid<X, Y, std::greater_equal<>>;

		// bitshift operators
		template <typename X, typename Y>
		using has_bit_xor = details::op_valid<X, Y, std::bit_xor<>>;
		template <typename X, typename Y>
		using has_bit_or = details::op_valid<X, Y, std::bit_or<>>;
		template <typename X, typename Y>
		using has_bit_and = details::op_valid<X, Y, std::bit_and<>>;
		template <typename X, typename Y>
		using has_bit_complement = details::op_valid<X, Y, std::bit_not<>>;
		template <typename X, typename Y>
		using has_bit_not = has_bit_complement<X, Y>;
		template <typename X, typename Y>
		using has_left_shift = details::op_valid<X, Y, details::left_shift>;
		template <typename X, typename Y>
		using has_right_shift = details::op_valid<X, Y, details::right_shift>;

		// increment/decrement operators
		template <typename X>
		using has_pre_increment = details::has_pre_increment<X>;
		template <typename X>
		using has_post_increment = details::has_post_increment<X>;
		template <typename X>
		using has_pre_decrement = details::has_pre_decrement<X>;
		template <typename X>
		using has_post_decrement = details::has_post_decrement<X>;
		template <typename X>
		using has_bool_operator = details::has_bool_operator<X>;

		// subscript operator
		template <typename T, typename Index>
		using has_subscript_operator = typename details::has_subscript_operator<T, Index>::type;
		template <typename T, typename Index>
		using has_bracket_operator = typename has_subscript_operator<T, Index>::type;

		// arithmetic operators
		template <typename T, typename U>
		using has_arithmetic_plus = typename details::has_arithmetic_plus<T, U>::type;
		template <typename T, typename U>
		using has_arithmetic_minus = typename details::has_arithmetic_minus<T, U>::type;
		template <typename T, typename U>
		using has_arithmetic_multiply = typename details::has_arithmetic_multiply<T, U>::type;
		template <typename T, typename U>
		using has_arithmetic_divide = typename details::has_arithmetic_divide<T, U>::type;
		template <typename T, typename U>
		using has_arithmetic_modulus = typename details::has_arithmetic_modulus<T, U>::type;

		// assignment operator
		template <typename X, typename Y>
		using has_assignment = std::is_assignable<X, Y>;

		// compound assignment operators
		template <typename T, typename U>
		using has_assignment_plus = typename details::has_assignment_plus<T, U>::type;
		template <typename T, typename U>
		using has_assignment_minus = typename details::has_assignment_minus<T, U>::type;
		template <typename T, typename U>
		using has_assignment_multiply = typename details::has_assignment_multiply<T, U>::type;
		template <typename T, typename U>
		using has_assignment_divide = typename details::has_assignment_divide<T, U>::type;
		template <typename T, typename U>
		using has_assignment_modulus = typename details::has_assignment_modulus<T, U>::type;
		template <typename T, typename U>
		using has_assignment_and = typename details::has_assignment_and<T, U>::type;
		template <typename T, typename U>
		using has_assignment_or = typename details::has_assignment_or<T, U>::type;
		template <typename T, typename U>
		using has_assignment_xor = typename details::has_assignment_xor<T, U>::type;
		template <typename T, typename U>
		using has_assignment_left_shift = typename details::has_assignment_left_shift<T, U>::type;
		template <typename T, typename U>
		using has_assignment_right_shift = typename details::has_assignment_right_shift<T, U>::type;

	}	 // namespace operators


	// todo move these helpers to a better location

	template <typename T>
	// struct func_traits : public func_traits<decltype(&decltype(std::function(std::declval<T>()))::operator()) >
	struct func_traits : public func_traits<decltype(&T::operator())>
	{};
	template <typename T>
	struct func_traits<std::function<T>> : public func_traits<T>
	{};
	template <typename T>
	struct func_traits<std::function<T>&> : public func_traits<T>
	{};
	template <typename T>
	struct func_traits<const std::function<T>&> : public func_traits<T>
	{};
	template <typename C, typename Ret, typename... Args>
	struct func_traits<Ret (C::*)(Args...)>
	{
		using result_t	  = Ret;
		using arguments_t = std::tuple<Args...>;
	};

	template <typename Ret, typename... Args>
	struct func_traits<Ret (*)(Args...)>
	{
		using result_t	  = Ret;
		using arguments_t = std::tuple<Args...>;
	};

	template <typename C, typename Ret, typename... Args>
	struct func_traits<Ret (C::*)(Args...) const>
	{
		using result_t	  = Ret;
		using arguments_t = std::tuple<Args...>;
	};

	template <typename T>
	struct is_invocable
	{
		template <typename U, typename = decltype(&T::operator())>
		static long test(const U&&);
		static char test(...);

		static constexpr bool value = sizeof(test(std::declval<T>())) == sizeof(long);
	};

	template <typename T>
	struct proxy_type
	{
		using type = T;
	};

	struct any
	{
		template <typename T>
		operator T&() const;

		template <typename T>
		operator T&&() const;
	};


	template <class, std::size_t N, class = std::make_index_sequence<N>, class = std::void_t<>>
	struct is_callable_n : std::false_type
	{};

	template <class F, std::size_t N, std::size_t... Idx>
	struct is_callable_n<F,
						 N,
						 std::index_sequence<Idx...>,
						 std::void_t<decltype(std::declval<F>()((Idx, std::declval<any const&&>())...))>> :
		std::true_type
	{};
}	 // namespace utility::templates

namespace psl::templates
{
	using namespace utility::templates;
}