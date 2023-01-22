#pragma once
#include "psl/details/fixed_astring.hpp"
#include "psl/ustring.hpp"
#include <stdexcept>
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

	template <IsParser T>
	using parser_result_type_t = typename parser_result_type<T>::type;

	template <IsParser Parser1, IsParser Parser2>
	struct is_compatible_parsers
		: std::conditional_t<std::is_same_v<parser_result_type_t<Parser1>, parser_result_type_t<Parser2>>,
							 std::true_type,
							 std::false_type> {};

	template <typename Parser1, typename Parser2>
	concept IsCompatibleParsers =
	  IsParser<Parser1> && IsParser<Parser2> && is_compatible_parsers<Parser1, Parser2>::value;
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
			return {res.view(), fmapFn(res.value())};
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

template <IsParser Parser, typename TransformFn>
constexpr auto bind(Parser&& parser, TransformFn&& transformFn) {
	return
	  [parser = std::forward<Parser>(parser), transformFn = std::forward<TransformFn>(transformFn)](parse_view_t view)
		-> std::invoke_result_t<TransformFn, typename parser_result_type_t<Parser>::value_type, parse_view_t> {
		  const auto result = parser(view);
		  if(!result) {
			  return invalid_result;
		  }
		  return transformFn(result.value(), result.view());
	  };
}

template <typename AccumulatorFn, IsParser Parser1, IsParser... Parsers>
requires((IsCompatibleParsers<Parser1, Parsers> && ...) &&
		 std::conjunction_v<std::is_same<parser_result_type_t<Parser1>, parser_result_type_t<Parsers>>...> &&
		 std::is_same_v<typename parser_result_type_t<Parser1>::value_type,
						std::invoke_result_t<AccumulatorFn,
											 typename parser_result_type_t<Parser1>::value_type,
											 typename parser_result_type_t<Parser1>::
											   value_type>>) constexpr auto accumulate(AccumulatorFn&& accumulatorFn,
																					   Parser1&& parser1,
																					   Parsers&&... parsers) noexcept {
	using return_type = typename parser_result_type_t<Parser1>::value_type;
	return [accumulatorFn = std::forward<AccumulatorFn>(accumulatorFn),
			parser1		  = std::forward<Parser1>(parser1),
			... parsers	  = std::forward<Parsers>(parsers)](parse_view_t view) -> parse_result_t<return_type> {
		parse_result_t<return_type> result = parser1(view);
		if(result) {
			(
			  [&result, &accumulatorFn](auto&& parser) mutable {
				  if(result) {
					  const auto next = parser(result.view());
					  if(next) {
						  result = {next.view(), accumulatorFn(result.value(), next.value())};
					  } else {
						  result = invalid_result;
					  }
				  }
			  }(parsers),
			  ...);
			return result;
		}
		return invalid_result;
	};
}

template <IsParser ParserLhs, IsParser ParserRhs>
requires(IsCompatibleParsers<ParserLhs, ParserRhs>) constexpr auto drop_left(ParserLhs&& lhs, ParserRhs&& rhs) {
	return accumulate([]<typename T>(T const&, T const& rhs) { return rhs; },
					  std::forward<ParserLhs>(lhs),
					  std::forward<ParserRhs>(rhs));
}

template <IsParser ParserLhs, IsParser ParserRhs>
requires(IsCompatibleParsers<ParserLhs, ParserRhs>) constexpr auto drop_right(ParserLhs&& lhs, ParserRhs&& rhs) {
	return accumulate([]<typename T>(T const& lhs, T const&) { return lhs; },
					  std::forward<ParserLhs>(lhs),
					  std::forward<ParserRhs>(rhs));
}

template <IsParser ParserLhs, IsParser ParserRhs>
requires(IsCompatibleParsers<ParserLhs, ParserRhs>) constexpr auto operator<(ParserLhs&& lhs, ParserRhs&& rhs) {
	return drop_left(lhs, rhs);
}

template <IsParser ParserLhs, IsParser ParserRhs>
requires(IsCompatibleParsers<ParserLhs, ParserRhs>) constexpr auto operator>(ParserLhs&& lhs, ParserRhs&& rhs) {
	return drop_right(lhs, rhs);
}

template <IsParser Parser1, IsParser Parser2>
requires(IsCompatibleParsers<Parser1, Parser2>) constexpr auto operator|(Parser1&& parser1,
																		 Parser2&& parser2) noexcept {
	return if_else(std::forward<Parser1>(parser1), std::forward<Parser2>(parser2));
}

template <IsParser Parser, typename T, typename Accumulator>
constexpr auto many(Parser&& parser, T&& default_val, Accumulator&& accumulator, size_t atleast = 0) {
	using return_type = parse_result_t<std::invoke_result_t<Accumulator, T, parser_result_type_t<Parser>>>;

	return [parser		= std::forward<Parser>(parser),
			default_val = std::forward<T>(default_val),
			accumulator = std::forward<Accumulator>(accumulator),
			atleast](parse_view_t view) -> return_type {
		auto result {default_val};
		size_t count {0};
		while(!view.empty()) {
			const auto intermediate = parser(view);
			if(!intermediate) {
				if(count >= atleast) {
					return {view, result};
				} else {
					return invalid_result;
				}
			}
			result = accumulator(result, intermediate.value());
			view   = intermediate.view();
			++count;
		}
		if(count >= atleast) {
			return {view, result};
		} else {
			return invalid_result;
		}
	};
}

template <IsParser Parser, typename T, typename Accumulator>
constexpr auto exactly(size_t exactly, Parser&& parser, T&& default_val, Accumulator&& accumulator) {
	using return_type = parse_result_t<std::invoke_result_t<Accumulator, T, T>>;

	return [parser		= std::forward<Parser>(parser),
			default_val = std::forward<T>(default_val),
			accumulator = std::forward<Accumulator>(accumulator),
			exactly](parse_view_t view) -> return_type {
		auto result {default_val};
		auto remaining = exactly;
		while(!view.empty() && remaining > 0) {
			const auto intermediate = parser(view);
			if(!intermediate) {
				return invalid_result;
			}
			result = accumulator(result, intermediate.value());
			view   = intermediate.view();
			--remaining;
		}
		if(remaining == 0) {
			return {view, result};
		} else {
			return invalid_result;
		}
	};
}

template <template <typename> typename Comparison = std::equal_to>
constexpr auto char_parser(char c) {
	return [c](parse_view_t view) -> parse_result_t<char> {
		if(!view.empty() && Comparison<char> {}(view.at(0), c)) {
			return {view.substr(1), c};
		}
		return invalid_result;
	};
}

template <template <typename> typename Comparison = std::equal_to>
constexpr auto char_compare_to(psl::string8::view characters) {
	return [characters](parse_view_t view) -> parse_result_t<char> {
		if(!view.empty() && Comparison<size_t> {}(characters.find(view.at(0)), psl::string8::view::npos)) {
			return {view.substr(1), view.at(0)};
		}
		return invalid_result;
	};
}

constexpr auto any_of(psl::string8::view characters) {
	return char_compare_to<std::not_equal_to>(characters);
}

constexpr auto none_of(psl::string8::view characters) {
	return char_compare_to<std::equal_to>(characters);
}

template <template <typename> typename Comparison = std::equal_to>
constexpr auto text_parser(psl::string8::view text) {
	return [text](parse_view_t view) -> parse_result_t<psl::string8::view> {
		if(Comparison<psl::string8::view::const_iterator> {}(
			 std::mismatch(text.begin(), text.end(), view.begin(), view.end()).first, text.end())) {
			return {view.substr(text.size()), text};
		}
		return invalid_result;
	};
}

constexpr auto skip_whitespace_parser() {
	return [](parse_view_t view) -> parse_result_t<psl::string8::view> {
		return fmap([&view](size_t value) -> psl::string8::view { return view.substr(0, value); },
					many(char_parser(' ') | char_parser('\r') | char_parser('\n') | char_parser('\t'),
						 size_t {0},
						 [](size_t result, char value) { return result + 1; }))(view);
	};
}

template <IsParser Parser>
constexpr auto accumulate_many(Parser&& parser, size_t atleast = 0) {
	return [parser = std::forward<Parser>(parser), atleast](parse_view_t view) -> parse_result_t<psl::string8::view> {
		return fmap([&view](size_t value) -> psl::string8::view { return view.substr(0, value); },
					many(
					  parser, size_t {0}, [](size_t result, char value) { return result + 1; }, atleast))(view);
	};
}

constexpr auto accumulate_many() {
	return [](parse_view_t view) -> parse_result_t<psl::string8::view> {
		return fmap([&view](size_t value) -> psl::string8::view { return view.substr(0, value); },
					many(
					  [](parse_view_t view) -> parse_result_t<char> {
						  if(view.empty()) {
							  return invalid_result;
						  }
						  return {view.substr(1), view.at(0)};
					  },
					  size_t {0},
					  [](size_t result, char value) { return result + 1; }))(view);
	};
}

template <IsParser Parser>
constexpr auto parse_to_string_view(Parser&& parser) {
	return [parser = std::forward<Parser>(parser)](parse_view_t view) -> parse_result_t<psl::string8::view> {
		if constexpr(std::is_same_v<parse_result_t<char>, parser_result_type_t<Parser>>) {
			if(auto result = parser(view); result) {
				return {result.view(), view.substr(0, 1)};
			}
		} else if constexpr(std::is_same_v<parse_result_t<psl::string8::view>, parser_result_type_t<Parser>>) {
			if(auto result = parser(view); result) {
				return {result.view(), result.value()};
			}
		} else {
			throw std::exception();
		}
		return invalid_result;
	};
}

template <psl::details::fixed_astring Message, IsParser Parser>
constexpr auto parse_or_throw(Parser&& parser) {
	return [parser = std::forward<Parser>(parser)](parse_view_t view) {
		if(const auto result = parser(view); result) {
			return result;
		} else {
			throw std::runtime_error {Message};
		}
	};
}

}	 // namespace psl::serialization::parser
