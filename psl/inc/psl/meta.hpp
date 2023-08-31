#pragma once
#include "psl/platform_def.hpp"
#include "psl/ustring.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace std {
#ifdef _MSC_VER
template <typename T>
struct hash;
#endif
}	 // namespace std

namespace psl {
/// \brief is an object holding a Unique IDentifier (UID)
///
/// UID generates a unique ID, either through a random number generator, or by using OS provided
/// methods. It is immutable once created.
struct UID final {
  public:
	friend struct std::hash<UID>;
	using PUID = std::array<uint8_t, 16>;

	UID(const psl::string8_t& key);

	/// \brief constructor that creates an invalid UID.
	UID() = default;
	/// a constructor that created a UID from the internal representation (PUID).
	/// \param[in] id the internal representation of a UID.
	constexpr UID(const PUID& id) : GUID(id) {}

	UID(const UID& other) noexcept			  = default;
	UID(UID&& other) noexcept				  = default;
	UID& operator=(const UID& other) noexcept = default;
	UID& operator=(UID&& other) noexcept	  = default;

	/// \brief creates a string based representation of the owned UID.
	///
	/// creates a string based representation of the owned UID of the form "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
	/// \returns a UTF-8 string based representation of the owned UID.
	const psl::string8_t to_string() const;

	/// \brief tries to convert from the given string based representation to a valid UID.
	///
	/// when sending a string of the form "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" or
	/// "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" to this method, it will parse it and will return a valid UID
	/// instance. Otherwise it will return UID::invalid_uid. \param[in] key a string in the valid format to convert
	/// to UID. \returns either a valid UID based on the key, or UID::invalid_uid if something went wrong.
	inline static UID from_string(const psl::string8_t& key);
	inline static UID from_string(psl::string8::view key);

	/// \brief checks the given string if it's in a valid UID format.
	/// \param[in] key the string to be checked.
	/// \returns true if the given key is convertible to a UID instance.
	inline static bool valid(const psl::string8_t& key) noexcept;

	/// \brief generates a UID.
	/// \returns a valid UID.
	static UID generate();

	bool operator==(const UID& b) const;
	bool operator!=(const UID& b) const;
	bool operator<(const UID& b) const;
	bool operator<=(const UID& b) const;
	bool operator>(const UID& b) const;
	bool operator>=(const UID& b) const;

	/// \brief checks if the held UID is valid.
	/// \returns true in case the held UID is valid.
	operator bool() const { return *this != invalid_uid; }

	/// \brief invalidates the current UID.
	///
	/// If for some reason you want to invalidate this object as being a valid UID, then calling this
	/// method will set the internal UID to be equivalent to UID::invalid_uid.
	/// This operation is permanent.
	void invalidate() { *this = invalid_uid; }

	/// \brief a global instance that signifies the invalid_uid used by the current runtime.
	/// \note on certain platforms this can be a "nulled" UID, but prefer checking against this
	/// instance to make sure that is actually the case. The invalid UID will not change during the life
	/// of the application.
	const static UID invalid_uid;

  private:
	PUID GUID;
};

constexpr bool valid_uid(const char* text, std::size_t size) noexcept {
	constexpr const size_t short_size = 36;
	constexpr const size_t long_size  = 38;
	if(size != short_size && size != long_size) {
		return false;
	}

	if(text[0] == '{') {
		if(text[size - 1] != '}') {
			return false;
		}
		text += 1;
	}

	if((text[8] != '-') || (text[13] != '-') || (text[18] != '-') || (text[23] != '-')) {
		return false;
	}

	constexpr auto parse = [](const char* text) {
		for(size_t i = 0; i < 2; ++i) {
			auto character = text[i];
			if(!('0' <= character && character <= '9') && !('a' <= character && character <= 'f') &&
			   !('A' <= character && character <= 'F'))
				return false;
		}
		return true;
	};

	for(size_t i = 0; i < 4; ++i) {
		if(!parse(text))
			return false;
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		if(!parse(text))
			return false;
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		if(!parse(text))
			return false;
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		if(!parse(text))
			return false;
		text += 2;
	}
	text += 1;

	for(size_t i = 0; i < 6; ++i) {
		if(!parse(text))
			return false;
		text += 2;
	}

	return true;
}
constexpr psl::UID try_make_uid(const char* text, std::size_t size) {
	constexpr const size_t short_size = 36;
	constexpr const size_t long_size  = 38;
	if(size != short_size && size != long_size) {
		throw std::domain_error("parsed text is of incorrect size to be a UID");
	}

	if(text[0] == '{') {
		if(text[size - 1] != '}') {
			throw std::domain_error("parsed text is of incorrect format to be a UID");
		}
		text += 1;
	}


	if((text[8] != '-') || (text[13] != '-') || (text[18] != '-') || (text[23] != '-')) {
		throw std::domain_error("parsed text is of incorrect format to be a UID");
	}

	constexpr auto parse = [](const char* text) {
		uint8_t result {};
		for(size_t i = 0; i < 2; ++i) {
			auto character = text[i];
			int res {};
			if('0' <= character && character <= '9')
				res = character - '0';
			else if('a' <= character && character <= 'f')
				res = 10 + character - 'a';
			else if('A' <= character && character <= 'F')
				res = 10 + character - 'A';
			else
				throw std::domain_error {"invalid character in UID"};

			result |= res << (4 * (1 - i));
		}
		return result;
	};

	psl::UID::PUID res {};
	auto res_offset = 0;
	for(size_t i = 0; i < 4; ++i) {
		res[res_offset++] = parse(text);
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		res[res_offset++] = parse(text);
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		res[res_offset++] = parse(text);
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		res[res_offset++] = parse(text);
		text += 2;
	}
	text += 1;

	for(size_t i = 0; i < 6; ++i) {
		res[res_offset++] = parse(text);
		text += 2;
	}
	return psl::UID {res};
}
constexpr psl::UID make_uid(const char* text, std::size_t size) noexcept {
	constexpr const size_t short_size = 36;
	constexpr const size_t long_size  = 38;
	if(size != short_size && size != long_size) {
		return psl::UID::invalid_uid;
	}

	if(text[0] == '{') {
		if(text[size - 1] != '}') {
			return psl::UID::invalid_uid;
		}
		text += 1;
	}


	if((text[8] != '-') || (text[13] != '-') || (text[18] != '-') || (text[23] != '-')) {
		return psl::UID::invalid_uid;
	}

	constexpr auto parse = [](const char* text, uint8_t& out) {
		for(size_t i = 0; i < 2; ++i) {
			auto character = text[i];
			int res {};
			if('0' <= character && character <= '9')
				res = character - '0';
			else if('a' <= character && character <= 'f')
				res = 10 + character - 'a';
			else if('A' <= character && character <= 'F')
				res = 10 + character - 'A';
			else
				return false;

			out |= res << (4 * (1 - i));
		}
		return true;
	};


	psl::UID::PUID res {};
	auto res_offset = 0;
	for(size_t i = 0; i < 4; ++i) {
		parse(text, res[res_offset++]);
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		parse(text, res[res_offset++]);
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		parse(text, res[res_offset++]);
		text += 2;
	}
	text += 1;
	for(size_t i = 0; i < 2; ++i) {
		parse(text, res[res_offset++]);
		text += 2;
	}
	text += 1;

	for(size_t i = 0; i < 6; ++i) {
		parse(text, res[res_offset++]);
		text += 2;
	}
	return psl::UID {res};
}

inline psl::UID psl::UID::from_string(const psl::string8_t& key) {
	return psl::make_uid(key.data(), key.size());
}

inline psl::UID psl::UID::from_string(psl::string8::view key) {
	return psl::make_uid(key.data(), key.size());
}

inline bool psl::UID::valid(const psl::string8_t& key) noexcept {
	return psl::valid_uid(key.data(), key.size());
}
}	 // namespace psl


// required by the natvis file
namespace dummy {
struct hex_dummy_low {
	unsigned char c;
};

struct hex_dummy_high {
	unsigned char c;
};
}	 // namespace dummy

namespace std {
template <>
struct hash<psl::UID> {
	size_t operator()(const psl::UID& x) const noexcept {
#if defined(PE_ARCHITECTURE_X86)
		const uint32_t* quarters = reinterpret_cast<const uint32_t*>(&x.GUID);
		return quarters[0] ^ quarters[1] ^ quarters[2] ^ quarters[3];
#elif defined(PE_ARCHITECTURE_X86_64)
		const uint64_t* half = reinterpret_cast<const uint64_t*>(&x.GUID);
		return half[0] ^ half[1];
#endif
	}
};
}	 // namespace std

constexpr psl::UID operator"" _uid(const char* text, std::size_t size) {
	return psl::try_make_uid(text, size);
}
