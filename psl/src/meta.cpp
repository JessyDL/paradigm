#include "psl/meta.hpp"
#include <random>

using namespace psl;

const UID UID::invalid_uid = storage_type {0};

UID::UID(const psl::string8_t& key) : m_Data(from_string(key).m_Data) {}

bool UID::operator==(const UID& b) const noexcept {
	return m_Data == b.m_Data;
}
bool UID::operator!=(const UID& b) const noexcept {
	return m_Data != b.m_Data;
}
bool UID::operator<(const UID& b) const noexcept {
	return m_Data < b.m_Data;
}
bool UID::operator>(const UID& b) const noexcept {
	return m_Data > b.m_Data;
}
bool UID::operator<=(const UID& b) const noexcept {
	return m_Data <= b.m_Data;
}
bool UID::operator>=(const UID& b) const noexcept {
	return m_Data >= b.m_Data;
}

const psl::string8_t UID::to_string() const {
	psl::string8_t str;
	str.resize(37);
	snprintf(str.data(),
			 str.size(),
			 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			 m_Data[0],
			 m_Data[1],
			 m_Data[2],
			 m_Data[3],
			 m_Data[4],
			 m_Data[5],
			 m_Data[6],
			 m_Data[7],
			 m_Data[8],
			 m_Data[9],
			 m_Data[10],
			 m_Data[11],
			 m_Data[12],
			 m_Data[13],
			 m_Data[14],
			 m_Data[15]);
	str.resize(36);
	return str;
}

UID UID::generate() {
	static std::random_device rd;
	static std::uniform_int_distribution<uint64_t> dist(0, (uint64_t)(~0));

	storage_type res {0};
	uint64_t* my = reinterpret_cast<uint64_t*>(res.data());

	my[0] = dist(rd);
	my[1] = dist(rd);

	my[0] = (my[0] & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
	my[1] = (my[1] & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

	return res;
}
