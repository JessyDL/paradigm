#include "psl/ustring.h"
#include "utf8.h"


psl::string16_t psl::string8::to_string16_t(const psl::string8_t& s)
{
	psl::string16_t res;
	utf8::utf8to16(s.begin(), s.end(), back_inserter(res));
	return res;
}

psl::string8_t psl::string8::from_string16_t(const psl::string16_t& s)
{
	psl::string8_t res;
	utf8::utf16to8(s.begin(), s.end(), back_inserter(res));
	return res;
}

psl::string8_t psl::string8::from_string16_t(psl::string16::view s) { return from_string16_t(psl::string16_t(s)); }


psl::string8_t psl::string8::from_wstring(const std::wstring& s)
{
	psl::string8_t res;
	utf8::utf16to8(s.begin(), s.end(), back_inserter(res));
	return res;
}

psl::string8_t psl::string16::to_string8_t(const psl::string16_t& s)
{
	psl::string8_t res;
	utf8::utf16to8(s.begin(), s.end(), back_inserter(res));
	return res;
}

psl::string16_t psl::string16::from_string8_t(const psl::string8_t& s)
{
	psl::string16_t res;
	utf8::utf8to16(s.begin(), s.end(), back_inserter(res));
	return res;
}

psl::string16_t psl::string16::from_string8_t(psl::string8::view s) { return from_string8_t(psl::string8_t(s)); }


#if !defined(STRING_16_BIT)
psl::pstring_t psl::to_pstring(const psl::string8_t& s)
{
#if defined(UNICODE)
	psl::pstring_t res;
	utf8::utf8to16(s.begin(), s.end(), back_inserter(res));
#else
	psl::pstring_t res(s.begin(), s.end());
#endif
	return res;
}

psl::pstring_t psl::to_pstring(psl::string8::view s)
{
#if defined(UNICODE)
	psl::pstring_t res;
	utf8::utf8to16(s.begin(), s.end(), back_inserter(res));
#else
	psl::pstring_t res(s.begin(), s.end());
#endif
	return res;
}

psl::pstring_t psl::to_pstring(const psl::string16_t& s)
{
#if defined(UNICODE)
	psl::pstring_t res(s.begin(), s.end());
#else
	psl::pstring_t res;
	utf8::utf16to8(s.begin(), s.end(), back_inserter(res));
#endif
	return res;
}

psl::pstring_t psl::to_pstring(psl::string16::view s)
{
#if defined(UNICODE)
	psl::pstring_t res(s.begin(), s.end());
#else
	psl::pstring_t res;
	utf8::utf16to8(s.begin(), s.end(), back_inserter(res));
#endif
	return res;
}

#if defined(UNICODE)
psl::string8_t psl::to_string8_t(psl::platform::view s)
{
	psl::string8_t res;
	utf8::utf16to8(s.begin(), s.end(), back_inserter(res));
	return res;
}

psl::string8_t psl::to_string8_t(const psl::pstring_t& s)
{
	psl::string8_t res;
	utf8::utf16to8(s.begin(), s.end(), back_inserter(res));
	return res;
}
#endif
#endif