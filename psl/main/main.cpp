// --------------------------------------------------------------------------------------------------------------------
// This main file exists for development purposes, here you can easily test out new features etc.. that are only
// implemented in the [psl] library.
// To activate this main file invoke cmake with `-DPE_DEV_MAKE_PSL_EXE=ON` and a new binary named `psl_main` will be
// generated in the output's `bin` folder
// --------------------------------------------------------------------------------------------------------------------

#include "psl/serialization/format.hpp"

int main(int argc, char* argv[]) {
	using namespace psl::serialization::parser;
	using namespace psl;

	constexpr psl::string8::view text {"identifier : t<float> [inline] { 'some_value', '5', '''3  3 ''' };"};
	constexpr psl::string8::view text1 {"i : object { a : f {};  }; "};
	constexpr psl::string8::view text2 {"i : object { a : object { b : {f}; };  }; "};
	constexpr psl::string8::view text3 {"b : {f};"};
	constexpr psl::string8::view text4 {
	  "identifi : object<float> [inline{something}] { a : object { b : {''' ''', unguarded}; };  }; "};

	constexpr psl::string8::view text5 {"b : {''' ''', unguarded}; "};
	constexpr auto size = psl::serialization::format::size(text2);
	// constexpr auto size = psl::serialization::format::size::parse(text4);
	//// constexpr std::array<char, size.value()> storage {};
	// constexpr auto field = psl::serialization::format::parse_field(text);

	//// constexpr auto f = psl::serialization::format::parse<
	////   "identifi : object<float> [inline{something}] { a : object { b : {''' ''', unguarded}; };  };
	////   "_fixed_astring>();

	// constexpr auto f = psl::serialization::format::parse("identifi : object<float> {};"_ctstr);

	/*constexpr auto f2	  = psl::serialization::format::parsing (psl::details::fixed_astring {
	  "identifi : object<float> [inline{something}] { a : object { b : {''' ''', unguarded}; };  }; "});*/
	// constexpr auto f_view = f2.value().view();
	//  static_assert(field.value().name == "identifier"sv);
	//  static_assert(field.value().type.name == "t"sv);
	//  static_assert(field.value().value.value == "some_value   "sv);

	// std::printf("%.*s\n", static_cast<int>(value.value().size()), value.value().data());
	return 0;
}
