#include "meta.h"
#include <cwctype>

#if defined(PLATFORM_WINDOWS) && !defined(META_GENERIC_UUID)

UID::UID(const psl::string8_t& key) { ::UuidFromStringA((RPC_CSTR)key.c_str(), &GUID); }

UID::UID() : GUID(invalid_uid.GUID) {}

UID::UID(const PUID& id) : GUID(id) {}

UID::UID(const UID& other) noexcept : GUID(other.GUID) {}
UID::UID(UID&& other) noexcept : GUID(std::move(other.GUID)) { other.invalidate(); }

UID& UID::operator=(const UID& other) noexcept
{
	if(GUID != other.GUID)
	{
		GUID = other.GUID;
	}
	return *this;
}
UID& UID::operator=(UID&& other) noexcept
{
	if(GUID != other.GUID)
	{
		GUID = std::move(other.GUID);
		other.invalidate();
	}
	return *this;
}

const psl::string8_t UID::to_string() const
{
	psl::string8_t result;
	RPC_CSTR szUuid = NULL;
	if(::UuidToStringA(&GUID, &szUuid) == RPC_S_OK)
	{
		result = std::move((char*)szUuid);
	}
	return result;
}

bool UID::operator==(const UID& b) const { return GUID == b.GUID; }

bool UID::operator!=(const UID& b) const { return GUID != b.GUID; }

bool UID::operator<(const UID& b) const
{
	return GUID.Data1 < b.GUID.Data1 && GUID.Data2 < b.GUID.Data2 && GUID.Data3 < b.GUID.Data3 &&
		   GUID.Data4 < b.GUID.Data4;
}
bool UID::operator>(const UID& b) const
{
	return GUID.Data1 > b.GUID.Data1 && GUID.Data2 > b.GUID.Data2 && GUID.Data3 > b.GUID.Data3 &&
		   GUID.Data4 > b.GUID.Data4;
}
bool UID::operator<=(const UID& b) const
{
	return GUID.Data1 <= b.GUID.Data1 && GUID.Data2 <= b.GUID.Data2 && GUID.Data3 <= b.GUID.Data3 &&
		   GUID.Data4 <= b.GUID.Data4;
}
bool UID::operator>=(const UID& b) const
{
	return GUID.Data1 >= b.GUID.Data1 && GUID.Data2 >= b.GUID.Data2 && GUID.Data3 >= b.GUID.Data3 &&
		   GUID.Data4 >= b.GUID.Data4;
}

UID UID::generate()
{
	UUID GUID;
	UuidCreate(&GUID);
	CoCreateGuid(&GUID);
	return UID(GUID);
}

UID UID::convert(const psl::string8_t& key)
{
	RPC_STATUS status;
	PUID id;
	status = ::UuidFromStringA((RPC_CSTR)key.c_str(), &id);
	if(status == RPC_S_OK)
	{
		return UID(id);
	}
	return invalid_uid;
}

bool UID::valid(const psl::string8_t& key)
{
	RPC_STATUS status;
	PUID id;
	status = ::UuidFromStringA((RPC_CSTR)key.c_str(), &id);
	return status == RPC_S_OK;
}
#endif