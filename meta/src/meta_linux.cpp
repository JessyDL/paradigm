#include "meta.h"

#if defined(PLATFORM_LINUX) && !defined(META_GENERIC_UUID)

UID::UID(const psl::string8_t& key)
{
	uuid_parse(key.c_str(), GUID);
}

UID::UID()
{
	uuid_copy(GUID, invalid_uid.GUID);
}
UID::UID(const PUID& id)
{
	uuid_copy(GUID, id);
}

UID::UID(const UID& other) noexcept
{
	uuid_copy(GUID, other.GUID);
}	

UID::UID(UID&& other) noexcept
{
	uuid_copy(GUID, other.GUID);
	other.invalidate();
}	

UID& UID::operator=(const UID& other) noexcept
{
	if(GUID != other.GUID)
	{
		uuid_copy(GUID, other.GUID);
	}
	return *this;
}

UID& UID::operator=(UID&& other) noexcept
{
	if(GUID != other.GUID)
	{
		uuid_copy(GUID, other.GUID);
		other.invalidate();
	}
	return *this;
}

const psl::string8_t UID::to_string() const
{
	psl::string8_t result;
	result.resize(37);
	uuid_unparse(GUID, result.data());
	return result;
}

bool UID::operator==(const UID& b) const { return uuid_compare(GUID, b.GUID) == 0; }

bool UID::operator!=(const UID& b) const { return uuid_compare(GUID, b.GUID) != 0; }

bool UID::operator<(const UID& b) const { return uuid_compare(GUID, b.GUID) < 0; }
bool UID::operator>(const UID& b) const { return uuid_compare(GUID, b.GUID) > 0; }
bool UID::operator<=(const UID& b) const { return uuid_compare(GUID, b.GUID) <= 0; }
bool UID::operator>=(const UID& b) const { return uuid_compare(GUID, b.GUID) >= 0; }

UID UID::generate()
{
	uuid_t uuid;
	uuid_generate_random(uuid);
	return UID(uuid);
}

UID UID::convert(const psl::string8_t&key)
{
	PUID id;
	if(uuid_parse(key.c_str(), id) == 0)
		return UID(id);
	return invalid_uid;
}

bool UID::valid(const psl::string8_t &key)
{
	PUID id;
	return uuid_parse(key.c_str(), id) == 0;
}

#endif