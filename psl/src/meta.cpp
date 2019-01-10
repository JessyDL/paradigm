#include "meta.h"
#include <random>

using namespace psl;

const UID UID::invalid_uid = PUID{0};


struct uuid_components
{
	uint32_t a;
	uint16_t b;
	uint16_t c;
	uint8_t d[2];
	uint8_t e[6];
};

UID::UID(const psl::string8_t& key) : GUID(convert(key).GUID) {}
UID::UID() : GUID(invalid_uid.GUID) {}
UID::UID(const PUID& id) : GUID(id) {}

bool UID::operator==(const UID& b) const { return GUID == b.GUID; }
bool UID::operator!=(const UID& b) const { return GUID != b.GUID; }
bool UID::operator<(const UID& b) const { return GUID < b.GUID; }
bool UID::operator>(const UID& b) const { return GUID > b.GUID; }
bool UID::operator<=(const UID& b) const { return GUID <= b.GUID; }
bool UID::operator>=(const UID& b) const { return GUID >= b.GUID; }

const psl::string8_t UID::to_string() const
{
	psl::string8_t str;
	str.resize(37);
	snprintf(str.data(),str.size(),
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
		GUID[3], GUID[2], GUID[1], GUID[0], GUID[5], GUID[4], GUID[7], GUID[6],
		GUID[8], GUID[9], GUID[10], GUID[11], GUID[12], GUID[13], GUID[14], GUID[15]
	);
	str.resize(36);
	return str;

	/*uuid_components& result = *(uuid_components*)(GUID.data());
	psl::string8::stream s;
	s << std::hex << std::setfill('0')
		<< std::setw(8) << result.a
		<< "-"
		<< std::setw(4) << result.b
		<< "-"
		<< std::setw(4) << result.c
		<< "-"
		<< std::setw(2) << (uint16_t)result.d[0]
		<< std::setw(2) << (uint16_t)result.d[1]
		<< "-"
		<< std::setw(2) << (uint16_t)result.e[0]
		<< std::setw(2) << (uint16_t)result.e[1]
		<< std::setw(2) << (uint16_t)result.e[2]
		<< std::setw(2) << (uint16_t)result.e[3]
		<< std::setw(2) << (uint16_t)result.e[4]
		<< std::setw(2) << (uint16_t)result.e[5];

	return s.str();*/
}

UID UID::generate()
{
	static std::random_device rd;
	static std::uniform_int_distribution<uint64_t> dist(0, (uint64_t)(~0));

	PUID res{0};
	uint64_t* my = reinterpret_cast<uint64_t*>(res.data());

	my[0] = dist(rd);
	my[1] = dist(rd);

	my[0] = (my[0] & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
	my[1] = (my[1] & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

	return res;
}

constexpr int hex_to_uchar(const char c)
{
	using namespace std::string_literals;
	if('0' <= c && c <= '9')
		return c - '0';
	else if('a' <= c && c <= 'f')
		return 10 + c - 'a';
	else if('A' <= c && c <= 'F')
		return 10 + c - 'A';
	else
		throw std::domain_error{"invalid character in GUID"s};
}

template<class T>
constexpr T parse(const char *ptr)
{
	T result{};
	for(size_t i = 0; i < sizeof(T) * 2; ++i)
		result |= hex_to_uchar(ptr[i]) << (4 * ((sizeof(T) * 2) - i - 1));
	return result;
}


UID UID::convert(const psl::string8_t&key)
{
	PUID res{};

	const auto first_pos = key.find_first_not_of('{');
	const auto last_pos = utility::string::rfind_first_not_of(key, '}');

	if(first_pos == key.npos || last_pos == key.npos || last_pos < first_pos || last_pos - first_pos != 35)
		return invalid_uid;


	uuid_components& result = *(uuid_components*)(&res);
	auto begin = key.c_str();
	result.a = parse<uint32_t>(begin);
	begin += 8 + 1;
	result.b = parse<uint16_t>(begin);
	begin += 4 + 1;
	result.c = parse<uint16_t>(begin);
	begin += 4 + 1;
	result.d[0] = parse<uint8_t>(begin);
	begin += 2;
	result.d[1] = parse<uint8_t>(begin);
	begin += 2 + 1;
	for(size_t i = 0; i < 6; ++i)
		result.e[i] = parse<uint8_t>(begin + i * 2);

	return res;
}

bool UID::valid(const psl::string8_t &key)
{
	const auto first_pos = key.find_first_not_of('{');
	const auto last_pos = utility::string::rfind_first_not_of(key, '}');

	if(first_pos == key.npos || last_pos == key.npos || last_pos < first_pos || last_pos - first_pos != 35)
		return false;

	for(size_t i = first_pos; i < last_pos; i += 2)
	{
		if(key[i] == '-')
			i += 1;

		const auto& ch0 = key[i];
		const auto& ch1 = key[i + 1];

		if(!isxdigit(ch0) || !isxdigit(ch1) || hex_to_uchar(ch0) == 0 || hex_to_uchar(ch1) == 0)
			return false;
	}

	return true;
}