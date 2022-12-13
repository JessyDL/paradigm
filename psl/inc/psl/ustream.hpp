#pragma once
#include "ustring.hpp"
#include <fstream>
#include <sstream>

namespace psl
{
/// \brief UTF-8 class namespace that contains the typedefs for the string objects, as well as helper methods.
namespace string8
{
	using stream   = std::basic_stringstream<char_t>;
	using ostream  = std::basic_ostream<char_t>;
	using ofstream = std::basic_ofstream<char_t>;
	using istream  = std::basic_istream<char_t>;
	using ifstream = std::basic_ifstream<char_t>;
	using fstream  = std::basic_fstream<char_t>;
}	 // namespace string8

/// \brief UTF-16 class namespace that contains the typedefs for the string objects, as well as helper methods.
namespace string16
{
	using stream   = std::basic_stringstream<char_t>;
	using ostream  = std::basic_ostream<char_t>;
	using ofstream = std::basic_ofstream<char_t>;
	using istream  = std::basic_istream<char_t>;
	using ifstream = std::basic_ifstream<char_t>;
	using fstream  = std::basic_fstream<char_t>;

}	 // namespace string16

/// \brief UTF-32 class namespace that contains the typedefs for the string objects, as well as helper methods.
namespace string32
{
	using stream   = std::basic_stringstream<char_t>;
	using ostream  = std::basic_ostream<char_t>;
	using ofstream = std::basic_ofstream<char_t>;
	using istream  = std::basic_istream<char_t>;
	using ifstream = std::basic_ifstream<char_t>;
	using fstream  = std::basic_fstream<char_t>;
}	 // namespace string32

namespace platform
{
	using stream   = std::basic_stringstream<char_t>;
	using ostream  = std::basic_ostream<char_t>;
	using ofstream = std::basic_ofstream<char_t>;
	using istream  = std::basic_istream<char_t>;
	using ifstream = std::basic_ifstream<char_t>;
	using fstream  = std::basic_fstream<char_t>;
}	 // namespace platform

using string8_t	 = std::basic_string<string8::char_t, std::char_traits<string8::char_t>>;
using string16_t = std::basic_string<string16::char_t, std::char_traits<string16::char_t>>;
using string32_t = std::basic_string<string32::char_t, std::char_traits<string32::char_t>>;

/// \brief platform conditional string type, turns into char or wchar depending on the platform and compile options
#if defined(STRING_16_BIT)
using string_view  = string16::view;
using stringstream = string16::stream;
using ostream	   = string16::ostream;
using ofstream	   = string16::ofstream;
using istream	   = string16::istream;
using ifstream	   = string16::ifstream;
using fstream	   = string16::fstream;
#else
	#if !defined(STRING_8_BIT)
		#define STRING_8_BIT
	#endif
using stringstream = string8::stream;
using ostream	   = string8::ostream;
using ofstream	   = string8::ofstream;
using istream	   = string8::istream;
using ifstream	   = string8::ifstream;
using fstream	   = string8::fstream;
#endif
}	 // namespace psl