#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/assertions.hpp"
#include "psl/static_array.hpp"
#include "psl/ustring.hpp"

#include "psl/string_utils.hpp"

#include <fmt/format.h>

#include "psl/serialization/details/parser.hpp"

//
//namespace psl::serialization::format {
//class container_t {};
//
//inline namespace {
//	consteval auto parser_directive_ct(psl::string8::view str) -> std::pair<bool, psl::string8::view> {
//		using namespace psl::serialization::parser;
//
//		/*auto directive = (make_string_parser("#"sv) <
//						 bind(
//						   any_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"sv),
//						   [](char x, parse_input_t rest) {
//							   static constexpr size_t count = [](parse_input_t rest) -> size_t {
//								   const auto count =
//									 many(any_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"sv),
//										  size_t {0},
//										  [](size_t acc, char c) -> size_t { return acc + 1; })(rest);
//								   if(!count) {
//									   throw "ParseError: could not parse the directive";
//								   }
//								   return count.value().first;
//							   }(rest);
//
//							   std::array<char, count> res;
//							   using type = std::pair<std::reference_wrapper<std::array<char, count>>, size_t>;
//
//							   return many(any_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"sv),
//										   type {std::ref(res), 0},
//										   [](type acc, char c) {
//											   acc.first.get()[acc.second++] = c;
//											   return acc;
//										   })(rest);
//						   }))(str);*/
//		//return directive;
//		return std::make_pair(false, psl::string8::view {});
//	}
//	
//	constexpr auto parser_directive_rt(psl::string8::view str)->std::pair<bool, psl::string8::view> {
//		using namespace psl::serialization::parser;
//		auto directive = make_string_parser("#"sv) <
//					   bind(
//						 any_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"sv),
//						 [](char x, parse_input_t rest) {
//							 const auto count =
//							   many(any_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"sv),
//									size_t {0},
//									[](size_t acc, char c) -> size_t { return acc + 1; })(rest);
//							 if(!count) {
//								 throw "ParseError: could not parse the directive";
//							 }
//
//							 psl::string8_t res;
//							 res.resize(count.value().first);
//							 using type = std::pair<std::reference_wrapper<psl::string8_t>, size_t>;
//
//							 return many(any_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"sv),
//										 type {std::ref(res), 0},
//										 [](type acc, char c) {
//											 acc.first.get()[acc.second++] = c;
//											 return acc;
//										 })(rest);
//						 });
//		auto res = directive(str);
//		return std::make_pair(res.has_value(), res.value().second);
//	}
//	constexpr auto parser_directive(const psl::string8::view str) -> std::pair<bool, psl::string8::view> {
//		using namespace psl::serialization::parser;
//		return parser_directive_rt(str);
//	}
//}	 // namespace
//
//template<typename = void>
//auto parse(psl::string8::view str)
//	requires(!std::is_constant_evaluated())
//{
//	using namespace psl::serialization::parser;
//
//	return parser_directive_rt(str);
//}
//
//template <typename = void>
//consteval auto parse(psl::string8::view str) requires(std::is_constant_evaluated()) {
//	return parser_directive_ct(str);
//}
//}	 // namespace psl::serialization::format
