#pragma once
#include "psl/ustring.hpp"
#include <tuple>
#include <type_traits>

namespace psl::serialization::parser {
using parse_view_t = psl::string8::view;

inline namespace details {
	struct invalid_result_t {
		explicit constexpr invalid_result_t(int) noexcept {}
	};

	inline constexpr invalid_result_t invalid_result {int(0)};

	template <typename T>
	concept IsParser = std::is_invocable_v<T, parse_view_t>;

	template <IsParser T>
	struct parser_result_type {
		using type = std::invoke_result_t<T, parse_view_t>;
	};

	template <IsParser... Parsers>
	requires(sizeof...(Parsers) < 2)	// when 1 or 0 they are compatible by default
	  struct is_compatible_parsers : std::true_type {};

	template <IsParser Parser1, IsParser... Rest>
	struct is_compatible_parsers<Parser1, Rest...>
		: std::conditional_t<std::conjunction_v<std::is_same<typename parser_result_type<Parser1>::type,
															 typename parser_result_type<Rest>::type>...>,
							 std::true_type,
							 std::false_type> {};

	template <typename... Parsers>
	concept IsCompatibleParsers = is_compatible_parsers<Parsers...>::value;


	template <IsParser T, IsParser... Rest>
	requires(sizeof...(Rest) == 0 ||
			 IsCompatibleParsers<T, Rest...>) using parser_result_type_t = typename parser_result_type<T>::type;

	template <typename T, typename Y>
	struct make_tuple_with_element_count_impl {};

	template <typename T, size_t... Indices>
	struct make_tuple_with_element_count_impl<T, std::index_sequence<Indices...>> {
		template <size_t>
		using _type_repeater = T;

		using type = std::tuple<_type_repeater<Indices>...>;
	};

	template <typename T, size_t Count>
	struct make_tuple_with_element_count {
		using type = typename make_tuple_with_element_count_impl<T, decltype(std::make_index_sequence<Count>())>::type;
	};

	template <typename T, size_t Count>
	using make_tuple_with_element_count_t = typename make_tuple_with_element_count<T, Count>::type;
}	 // namespace details

template <typename T>
requires(std::is_default_constructible_v<T>) class parse_result_t {
  public:
	using value_type = T;

	constexpr parse_result_t() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
	constexpr parse_result_t(invalid_result_t) noexcept(std::is_nothrow_default_constructible_v<T>) {}
	template <typename... Args>
	constexpr parse_result_t(parse_view_t view, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
		: m_Value(std::forward<Args>(args)...), m_View(view) {}

	constexpr operator bool() const noexcept { return m_View.data() != nullptr; }

	constexpr T& value() noexcept { return m_Value; }
	constexpr T const& value() const noexcept { return m_Value; }
	constexpr parse_view_t& view() noexcept { return m_View; }
	constexpr parse_view_t const& view() const noexcept { return m_View; }

  private:
	T m_Value {};
	parse_view_t m_View {};
};

template <typename T>
parse_result_t(parse_view_t, T&&) -> parse_result_t<T>;

template <typename FMapFn, IsParser ParserFn>
constexpr auto fmap(FMapFn&& fmapFn, ParserFn&& parserFn) noexcept {
	return [fmapFn = std::forward<FMapFn>(fmapFn), parserFn = std::forward<ParserFn>(parserFn)](
			 parse_view_t view) -> parse_result_t<std::invoke_result_t<FMapFn, parser_result_type_t<ParserFn>>> {
		if(parse_result_t const res = parserFn(view); res) {
			return {fmapFn(res.value()), res.view()};
		}
		return invalid_result;
	};
}

template <typename T>
constexpr auto fail(T&&) {
	return [](parse_view_t) -> parse_result_t<T> { return invalid_result; };
}

template <typename T, typename ErrorFn>
constexpr auto fail(T&&, ErrorFn&& errorFn) {
	return [errorFn = std::forward<ErrorFn>(errorFn)](parse_view_t) -> parse_result_t<T> {
		errorFn();
		return invalid_result;
	};
}

template <IsParser Parser1, IsParser Parser2>
requires(IsCompatibleParsers<Parser1, Parser2>) constexpr auto if_else(Parser1&& parser1, Parser2&& parser2) noexcept {
	return [parser1 = std::forward<Parser1>(parser1),
			parser2 = std::forward<Parser2>(parser2)](parse_view_t view) -> parser_result_type_t<Parser1> {
		if(const parse_result_t res1 = parser1(view); res1) {
			return res1;
		}
		return parser2(view);
	};
}

template <IsParser... Parsers, typename AccumulatorFn>
requires(IsCompatibleParsers<Parsers...> &&
		 sizeof...(Parsers) > 1) constexpr auto accumulate(Parsers&&... parsers, AccumulatorFn&& accumulatorFn) {
	using parser_return_type	  = parser_result_type_t<Parsers...>;
	using accumulated_result_type = make_tuple_with_element_count_t<parser_return_type, sizeof...(Parsers)>;
	using return_type = parse_result_t<std::invoke_result_t<AccumulatorFn, parser_return_type, parser_return_type>>;

	return [parsers = std::forward<Parsers>(parsers)](parse_view_t view) -> return_type {
		return_type result {invalid_result};
		auto cpy_view = view;
		bool stop {false};
		accumulated_result_type accumulated {[&stop, &view]() mutable -> parser_return_type {
			if(!stop) {
				const auto result = Parsers(view);
				if(!result) {
					stop = true;
					return invalid_result;
				} else {
					view = result.view();
					return result;
				}
			}
		}()...};

		if(stop) {
			return invalid_result;
		}
		return {std::apply(AccumulatorFn, accumulated), view};
	};
}

template <IsParser ParserLhs, IsParser ParserRhs>
requires(IsCompatibleParsers<ParserLhs, ParserRhs>) constexpr auto drop_left(ParserLhs&& lhs, ParserRhs&& rhs) {
	return accumulate(std::forward<ParserLhs>(lhs), std::forward<ParserRhs>(rhs), []<typename T>(T&&, T&& rhs) {
		return std::forward<T>(rhs);
	});
}

template <IsParser ParserLhs, IsParser ParserRhs>
requires(IsCompatibleParsers<ParserLhs, ParserRhs>) constexpr auto drop_right(ParserLhs&& lhs, ParserRhs&& rhs) {
	return accumulate(std::forward<ParserLhs>(lhs), std::forward<ParserRhs>(rhs), []<typename T>(T&& lhs, T&&) {
		return std::forward<T>(lhs);
	});
}

template <IsParser Parser1, IsParser Parser2>
requires(IsCompatibleParsers<Parser1, Parser2>) constexpr auto operator|(Parser1&& parser1,
																		 Parser2&& parser2) noexcept {
	return if_else(std::forward<Parser1>(parser1), std::forward<Parser2>(parser2));
}
}	 // namespace psl::serialization::parser