#pragma once
#include "psl/assertions.hpp"
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace utility::templates
{
	inline namespace details
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

	template <typename... Ts>
	struct type_pack_t
	{};

	template <typename T>
	struct type_pack_size_t
	{};

	template <typename... Ts>
	struct type_pack_size_t<type_pack_t<Ts...>>
	{
		static constexpr size_t value {sizeof...(Ts)};
	};

	inline namespace details
	{
		template <typename... Ts>
		struct has_type_impl : std::false_type
		{};

		template <typename T, typename Y, typename... Ts>
		requires(!std::is_same_v<T, Y>) struct has_type_impl<T, Y, Ts...> : public has_type_impl<T, Ts...>
		{};

		template <typename T, typename Y, typename... Ts>
		requires(std::is_same_v<T, Y>) struct has_type_impl<T, Y, Ts...> : public std::true_type
		{};

	}	 // namespace details

	template <typename... Ts>
	struct has_type : has_type_impl<Ts...>
	{};

	template <typename T, typename... Ts>
	struct has_type<T, type_pack_t<Ts...>> : public has_type_impl<T, Ts...>
	{};

	template <typename T, typename... Ts>
	concept HasType = has_type<T, Ts...>::value;

	inline namespace details
	{
		template <typename... Ts>
		struct index_of_impl
		{};

		template <typename T, typename Y, typename... Ts>
		requires(!std::is_same_v<T, Y>) struct index_of_impl<T, Y, Ts...>
		{
			static constexpr std::size_t value = 1 + index_of_impl<T, Ts...>::value;
		};

		template <typename T, typename Y, typename... Ts>
		requires(std::is_same_v<T, Y>) struct index_of_impl<T, Y, Ts...>
		{
			static constexpr std::size_t value = 0;
		};
	}	 // namespace details

	template <typename T, typename... Ts>
	requires HasType<T, Ts...>
	struct index_of : public index_of_impl<T, Ts...>
	{};

	template <typename T, typename... Ts>
	requires HasType<T, Ts...>
	struct index_of<T, type_pack_t<Ts...>> : index_of_impl<T, Ts...>
	{};

	template <typename... Ts>
	static constexpr auto index_of_v = index_of<Ts...>::value;

	inline namespace details
	{
		template <size_t N, size_t Curr, typename... Ts>
		struct type_at_index_impl
		{};

		template <size_t N, size_t Curr, typename T, typename... Ts>
		requires(N == Curr) struct type_at_index_impl<N, Curr, T, Ts...>
		{
			using type = T;
		};

		template <size_t N, size_t Curr, typename T, typename... Ts>
		requires(N != Curr) struct type_at_index_impl<N, Curr, T, Ts...> : public type_at_index_impl<N, Curr + 1, Ts...>
		{};
	}	 // namespace details

	template <size_t N, typename... Ts>
	requires(N < sizeof...(Ts)) struct type_at_index : type_at_index_impl<N, 0, Ts...>
	{};

	template <size_t N, typename... Ts>
	struct type_at_index<N, type_pack_t<Ts...>> : type_at_index<N, Ts...>
	{};

	template <size_t N, typename... Ts>
	using type_at_index_t = typename type_at_index<N, Ts...>::type;

	// handy utility for the compilers that wish to compile static_assert(false,"") in dead code paths that should error
	// out
	template <typename T>
	struct always_false : std::false_type
	{};

	template <typename T>
	inline constexpr bool always_false_v = always_false<T>::value;

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
		inline namespace details
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
		using has_bracket_operator = typename details::has_subscript_operator<T, Index>::type;

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
		using arguments_t = type_pack_t<Args...>;
	};

	template <typename Ret, typename... Args>
	struct func_traits<Ret (*)(Args...)>
	{
		using result_t	  = Ret;
		using arguments_t = type_pack_t<Args...>;
	};

	template <typename C, typename Ret, typename... Args>
	struct func_traits<Ret (C::*)(Args...) const>
	{
		using result_t	  = Ret;
		using arguments_t = type_pack_t<Args...>;
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

namespace psl
{
	template <typename T, typename... Ts>
	using has_type = utility::templates::has_type<T, Ts...>;

	template <typename T, typename... Ts>
	concept HasType = has_type<T, Ts...>::value;

	template <typename... Ts>
	using index_of = utility::templates::index_of<Ts...>;

	template <typename... Ts>
	static constexpr auto index_of_v = index_of<Ts...>::value;

	template <size_t N, typename... Ts>
	using type_at_index = utility::templates::type_at_index<N, Ts...>;

	template <size_t N, typename... Ts>
	using type_at_index_t = typename type_at_index<N, Ts...>::type;

	template <typename... Ts>
	using type_pack_t = utility::templates::type_pack_t<Ts...>;

	template <typename T>
	static constexpr auto type_pack_size_v = utility::templates::type_pack_size_t<T>::value;
}	 // namespace psl