#include "psl/meta.hpp"
#include <random>

using namespace psl;

const UID UID::invalid_uid = PUID {0};

UID::UID(const psl::string8_t& key) : GUID(from_string(key).GUID) {}

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
	snprintf(str.data(),
			 str.size(),
			 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			 GUID[0],
			 GUID[1],
			 GUID[2],
			 GUID[3],
			 GUID[4],
			 GUID[5],
			 GUID[6],
			 GUID[7],
			 GUID[8],
			 GUID[9],
			 GUID[10],
			 GUID[11],
			 GUID[12],
			 GUID[13],
			 GUID[14],
			 GUID[15]);
	str.resize(36);
	return str;
}

UID UID::generate()
{
	static std::random_device rd;
	static std::uniform_int_distribution<uint64_t> dist(0, (uint64_t)(~0));

	PUID res {0};
	uint64_t* my = reinterpret_cast<uint64_t*>(res.data());

	my[0] = dist(rd);
	my[1] = dist(rd);

	my[0] = (my[0] & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
	my[1] = (my[1] & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

	return res;
}