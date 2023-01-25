#include "psl/serialization/format.hpp"

#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

#include <litmus/generator/range.hpp>

using namespace litmus;

struct text_data_t {
	psl::string8::view text;
	size_t identifiers;
	size_t identifiers_size;
	size_t values;
	size_t values_size;
	size_t types;
	size_t types_size;
	size_t directives;
	size_t directives_size;
	size_t directive_values;
	size_t directive_values_size;
	bool valid;
};

constexpr std::array data {
  text_data_t {
	.text				   = "identifier : object { val0 : {}; val1 : {'something'};};"sv,
	.identifiers		   = 3,
	.identifiers_size	   = 18,
	.values				   = 2,
	.values_size		   = 9,
	.types				   = 3,
	.types_size			   = 6,
	.directives			   = 0,
	.directives_size	   = 0,
	.directive_values	   = 0,
	.directive_values_size = 0,
	.valid				   = true,
  },
  text_data_t {
	.text = "identifier : object { obj0 : object { val0: {''' ''', '--' }; obj1 :{}; }; val1 : {'something'};};"sv,
	.identifiers		   = 5,
	.identifiers_size	   = 26,
	.values				   = 4,
	.values_size		   = 12,
	.types				   = 5,
	.types_size			   = 12,
	.directives			   = 0,
	.directives_size	   = 0,
	.directive_values	   = 0,
	.directive_values_size = 0,
	.valid				   = true,
  },
  text_data_t {
	.text =
	  "#version 1; identifier : object { obj0 : object { val0: {''' ''', '--' }; obj1 :{}; }; val1 : {'something'};}; #import hello.txt;"sv,
	.identifiers		   = 5,
	.identifiers_size	   = 26,
	.values				   = 4,
	.values_size		   = 12,
	.types				   = 5,
	.types_size			   = 12,
	.directives			   = 2,
	.directives_size	   = 13,
	.directive_values	   = 2,
	.directive_values_size = 10,
	.valid				   = true,
  },
  text_data_t {
	// directive statements within an object aren't allowed
	.text  = "obj : f { #version 5; }"sv,
	.valid = false,
  },
  text_data_t {
	// here we don't get to parse the entire view, which should result in an error.
	.text  = "#version 5; }"sv,
	.valid = false,
  },
};

template <size_t N>
struct vpack_for_array {
	using type = decltype([]<size_t... Indices>(std::index_sequence<Indices...>) {
		return vpack<Indices...> {};
	}(std::make_index_sequence<N> {}));
};

template <size_t N>
using vpack_for_array_t = typename vpack_for_array<N>::type;

namespace {
auto t0 = suite<"psl::serialization::format">().templates<vpack<true, false>, vpack_for_array_t<data.size()>>() =
  []<typename IsConstexpr, typename Index>() {
	  constexpr auto is_constexpr = IsConstexpr::value;
	  constexpr auto index		  = Index::value;

	  auto tests = [](auto const& result, auto const& data) {
		  expect(result) == data.valid;

		  // if the data is invalid, we don't expect any result to be correct beyond this point
		  if(!data.valid) {
			  return;
		  }

		  expect(result.value().identifiers.count) == data.identifiers;
		  expect(result.value().identifiers.size) == data.identifiers_size;
		  expect(result.value().values.count) == data.values;
		  expect(result.value().values.size) == data.values_size;
		  expect(result.value().types.count) == data.types;
		  expect(result.value().types.size) == data.types_size;
		  expect(result.value().directives.count) == data.directives;
		  expect(result.value().directives.size) == data.directives_size;
		  expect(result.value().directives_values.count) == data.directive_values;
		  expect(result.value().directives_values.size) == data.directive_values_size;
	  };

	  // no point in testing invalid data as it will result in a compile error
	  if constexpr(is_constexpr) {
		  if(data[index].valid) {
			  constexpr auto result = psl::serialization::format::size(data[index].text);
			  tests(result, data[index]);
		  }
	  } else {
		  auto result = psl::serialization::format::size(data[index].text);
		  tests(result, data[index]);
	  }
  };
}
