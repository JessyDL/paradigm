#include "psl/library.hpp"
#include "psl/assertions.hpp"
#include "psl/platform_utils.hpp"
#include "psl/serialization/encoder.hpp"
#include "psl/serialization/polymorphic.hpp"

using namespace psl::meta;
using namespace psl::serialization;
using namespace psl;
const uint64_t file::polymorphic_identity {register_polymorphic<file>()};

library::library(std::optional<psl::string8::view> lib, std::vector<psl::string8_t> environment) {
	if(!lib) {
		return;
	}

	m_LibraryLocation = psl::utility::platform::directory::to_platform(lib.value_or(""));

	if(auto loc = m_LibraryLocation.rfind(psl::to_string8_t(psl::utility::platform::directory::seperator));
	   loc == psl::string8_t::npos) {
		loc				= 0;
		m_LibraryFolder = ".";
		m_LibraryFile	= lib.value_or("");
	} else {
		m_LibraryFolder = psl::string8::view(&m_LibraryLocation[0], loc);
		m_LibraryFile =
		  psl::string8::view(&m_LibraryLocation[loc + psl::utility::platform::directory::seperator.size()],
							 m_LibraryLocation.size() - loc - psl::utility::platform::directory::seperator.size());
		psl_assert(psl::utility::platform::file::exists(psl::from_string8_t(m_LibraryLocation)),
				   "could not find library at '{}'",
				   m_LibraryLocation);
	}

	psl::string8_t root =
	  psl::string8_t(m_LibraryFolder) + psl::to_string8_t(psl::utility::platform::directory::seperator);
	// Load library into memory
	serializer s;
	metalib metalib;
	s.deserialize<decode_from_format>(metalib, lib.value());

	for(auto& entry : metalib.entries.value) {
		// here we handle environment variations
		// if the environments for the file aren't empty we check if any_of the environments are also in the provided
		// environment list, if not (or if the environment list is empty) we skip this entry
		if(!entry.environments->empty() &&
		   !std::any_of(std::begin(environment), std::end(environment), [&entry](auto const& environment) {
			   return std::find(entry.environments->begin(), entry.environments->end(), environment) !=
					  entry.environments->end();
		   })) {
			continue;
		}
		psl_assert(m_MetaData.find(entry.id) == std::end(m_MetaData),
				   "duplicate UID {} found in library",
				   entry.id->to_string());

		auto full_metapath = psl::utility::platform::file::to_platform(root + entry.meta->path);
		psl_assert(psl::utility::platform::file::exists(full_metapath),
				   "could not find file associated with UID {} at {}",
				   entry.id->to_string(),
				   entry.meta->path.value);

		file* metaPtr = nullptr;
		s.deserialize<decode_from_format>(metaPtr, full_metapath);

		psl_assert(metaPtr->ID() == entry.id.value,
				   "UID mismatch between library and metafile library expected {} but file has {}",
				   entry.id->to_string(),
				   metaPtr->ID().to_string());

		auto pair = m_MetaData.emplace(metaPtr->ID(), std::move(metaPtr));
		m_TagMap[entry.data->path].insert(pair.first->second.data->ID());
		pair.first->second.flags[0]		= true;
		pair.first->second.readableName = entry.data->path;
	}
}


library::~library() {}

bool library::serialize(const UID& uid) {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || it->second.flags[0] != true)
		return false;

	psl::string8_t filepath = psl::string8_t(m_LibraryFolder) +
							  psl::to_string8_t(psl::utility::platform::directory::seperator) + it->second.readableName;
	serializer s;
	s.serialize<encode_to_format>(it->second.data.get(), psl::string8::view {filepath});
	return true;
};

bool library::remove(const UID& uid, bool safe_mode) {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || (safe_mode && it->second.referencedBy.size() > 0))
		return false;

	// notify all those who are referencing me that I no longer exist
	for(auto& ref : it->second.referencedBy) {
		if(auto refIt = m_MetaData.find(ref); refIt != m_MetaData.end()) {
			refIt->second.referencing.erase(uid);
		}
	}

	// notify all those I'm referencing that I no longer exist
	for(auto& ref : it->second.referencing) {
		if(auto refIt = m_MetaData.find(ref); refIt != m_MetaData.end()) {
			refIt->second.referencedBy.erase(uid);
		}
	}
	// remove all tags
	for(auto& tag : it->second.data->m_Tags.value) {
		auto tagIt = m_TagMap.find(tag);
		if(tagIt == m_TagMap.end())
			continue;

		tagIt->second.erase(uid);
		if(tagIt->second.size() == 0)
			m_TagMap.erase(tagIt);
	}

	m_MetaData.erase(it);
	return true;
}

bool library::contains(const UID& uid) const {
	return m_MetaData.find(uid) != m_MetaData.end();
}


std::optional<UID> library::find(psl::string8::view tag) const {
	auto it = m_TagMap.find(psl::string8_t(tag));
	if(it == m_TagMap.end() || it->second.size() == 0)
		return {};

	return *std::begin(it->second);
}
std::unordered_set<UID> library::find_all(psl::string8::view tag) const {
	auto it = m_TagMap.find(psl::string8_t(tag));
	if(it == m_TagMap.end())
		return {};
	return it->second;
}

const std::vector<psl::string8_t>& library::tags(const UID& uid) const {
	static std::vector<psl::string8_t> empty;
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData))
		return empty;

	return it->second.data->m_Tags;
}

bool library::has_tag(const UID& uid, psl::string8::view tag) const {
	auto it = m_TagMap.find(psl::string8_t(tag));
	if(it == m_TagMap.end())
		return false;

	return it->second.find(uid) == it->second.end();
}

bool library::set(const UID& uid, psl::string8::view tag) {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData))
		return false;

	m_TagMap[psl::string8_t(tag)].insert(uid);
	it->second.data->m_Tags.value.push_back(psl::string8_t(tag));
	return true;
}
bool library::set(const UID& uid, std::vector<psl::string8::view> tags) {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData))
		return false;

	for(auto& tag : tags) {
		m_TagMap[psl::string8_t(tag)].insert(uid);
	}
	it->second.data->m_Tags.value.insert(it->second.data->m_Tags.value.end(), tags.begin(), tags.end());
	return true;
}

std::unordered_set<UID> library::referencing(const UID& uid) const {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData))
		return {};

	return it->second.referencing;
}
std::unordered_set<UID> library::referencedBy(const UID& uid) const {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData))
		return {};

	return it->second.referencedBy;
}

bool library::is_physical_file(const UID& uid) const {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData))
		return false;

	return it->second.flags[0] == true;
}
std::optional<psl::string8_t> library::get_physical_location(const UID& uid) const {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || it->second.flags[0] != true)
		return {};

	return psl::string8_t(m_LibraryFolder) + psl::to_string8_t(psl::utility::platform::directory::seperator) +
		   it->second.readableName;
}

size_t library::size() const {
	return m_MetaData.size();
}


std::optional<psl::string8::view> library::load(const UID& uid) {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData))
		return {};

	if(it->second.flags[0] != true || it->second.file_data.size() > 0)
		return it->second.file_data;

	if(auto res = psl::utility::platform::file::read(psl::from_string8_t(m_LibraryFolder) +
													 psl::utility::platform::directory::seperator +
													 psl::from_string8_t(it->second.readableName));
	   res) {
		it->second.file_data = psl::string(res.value().data(), res.value().size());
		return it->second.file_data;
	}
	return {};
}

bool library::unload(const UID& uid) {
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || it->second.flags[0] != true || it->second.file_data.empty())
		return false;

	it->second.file_data = {};
	return true;
}


void library::replace_content(psl::UID uid, psl::string8_t content) noexcept {
	if(auto it = m_MetaData.find(uid); it != std::end(m_MetaData)) {
		it->second.file_data = std::move(content);
	}
}
