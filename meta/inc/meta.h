#pragma once
#include "stdafx_psl.h"
#include "platform_def.h"
#include "string_utils.h"

namespace std
{
#ifdef _MSC_VER
	template<typename T>
	struct hash;
#endif
}

/// \brief is an object holding a Unique IDentifier (UID)
///
/// UID generates a unique ID, either through a random number generator, or by using OS provided
/// methods. It is immutable once created.
struct UID
{

public:
	UID(const psl::string8_t &key);

	friend struct std::hash<UID>;
	using PUID = std::array<uint8_t, 16>;



public:
	/// \brief constructor that creates a valid UID.
	UID();
	/// a constructor that created a UID from the internal representation (PUID).
	/// \param[in] id the internal representation of a UID.
	UID(const PUID& id);
	
	UID(const UID& other) noexcept = default;
	UID(UID&& other) noexcept = default;
	UID& operator=(const UID& other) noexcept = default;
	UID& operator=(UID&& other) noexcept = default;

	/// \brief creates a string based representation of the owned UID.
	///
	/// creates a string based representation of the owned UID of the form "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
	/// \returns a UTF-8 string based representation of the owned UID.
	const psl::string8_t to_string() const;

	/// \brief tries to convert from the given string based representation to a valid UID.
	///
	/// when sending a string of the form "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" or "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
	/// to this method, it will parse it and will return a valid UID instance. Otherwise it will return UID::invalid_uid.
	/// \param[in] key a string in the valid format to convert to UID.
	/// \returns either a valid UID based on the key, or UID::invalid_uid if something went wrong.
	static UID convert(const psl::string8_t& key);

	/// \brief checks the given string if it's in a valid UID format.
	/// \param[in] key the string to be checked.
	/// \returns true if the given key is convertible to a UID instance.
	static bool valid(const psl::string8_t& key);
	
	/// \brief generates a UID, it is equivalent to calling the default UID::UID() constructor.
	/// \returns a valid UID.
	static UID generate();
	
	bool operator == (const UID& b) const;
	bool operator != (const UID& b) const;
	bool operator < (const UID& b) const;
	bool operator <= (const UID& b) const;
	bool operator > (const UID& b) const;
	bool operator >= (const UID& b) const;

	/// \brief checks if the held UID is valid.
	/// \returns true in case the held UID is valid.
	operator bool() const
	{
		return *this != invalid_uid;
	}
	
	/// \brief invalidates the current UID.
	///
	/// If for some reason you want to invalidate this object as being a valid UID, then calling this
	/// method will set the internal UID to be equivalent to UID::invalid_uid.
	/// This operation is permanent.
	void invalidate()
	{
		*this = invalid_uid;
	}

	/// \brief a global instance that signifies the invalid_uid used by the current runtime. 
	/// \note on certain platforms this can be a "nulled" UID, but prefer checking against this
	/// instance to make sure that is actually the case. The invalid UID will not change during the life
	/// of the application.
	const static UID invalid_uid;
private:
	PUID GUID;
	
};

namespace std {
	template <> struct hash<UID>
	{
		size_t operator()(const UID & x) const
		{
			const uint64_t* half = reinterpret_cast<const uint64_t*>(&x.GUID);
			return half[0] ^ half[1];
		}
	};
}

namespace utility
{
	template<>
	struct converter<UID>
	{
		static psl::string8_t to_string(const UID& x)
		{
			return x.to_string();
		}

		static UID from_string(psl::string8::view str)
		{
			return UID::convert(psl::string8_t(str));
		}
	};
}
