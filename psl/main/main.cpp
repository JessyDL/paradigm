// --------------------------------------------------------------------------------------------------------------------
// This main file exists for development purposes, here you can easily test out new features etc.. that are only
// implemented in the [psl] library.
// To activate this main file invoke cmake with `-DPE_DEV_MAKE_PSL_EXE=ON` and a new binary named `psl_main` will be
// generated in the output's `bin` folder
// --------------------------------------------------------------------------------------------------------------------

#include "psl/serialization/format.hpp"

int main(int argc, char* argv[]) {
	using namespace psl::serialization::parser;

	constexpr psl::string8::view text {"identifier : t<float> { 'some_value', '5', '''3  3 ''' };"};

	constexpr auto size = psl::serialization::format::size::parse_field(text);
	// constexpr std::array<char, size.value()> storage {};
	// constexpr auto field = psl::serialization::format::parse_field(text);
	// static_assert(field.value().name == "identifier"sv);
	// static_assert(field.value().type.name == "t"sv);
	// static_assert(field.value().value.value == "some_value   "sv);

	// std::printf("%.*s\n", static_cast<int>(value.value().size()), value.value().data());
	return 0;
}
