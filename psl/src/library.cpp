#include "library.h"
#include "platform_utils.h"

using namespace psl::meta;
using namespace psl::serialization;
using namespace psl;
const uint64_t file::polymorphic_identity{register_polymorphic<file>()};

library::library(psl::string8::view lib)
{
	m_LibraryLocation = utility::platform::directory::to_platform(lib);
	auto loc		  = m_LibraryLocation.rfind(psl::to_string8_t(utility::platform::directory::seperator));
	m_LibraryFolder   = psl::string8::view(&m_LibraryLocation[0], loc);
	m_LibraryFile	 = psl::string8::view(&m_LibraryLocation[loc + utility::platform::directory::seperator.size()],
									   m_LibraryLocation.size() - loc - utility::platform::directory::seperator.size());

	psl::string8_t root = psl::string8_t(m_LibraryFolder) + psl::to_string8_t(utility::platform::directory::seperator);
	if(utility::platform::file::exists(psl::from_string8_t(m_LibraryLocation)))
	{
		// Load library into memory
		serializer s;

		std::ifstream library(m_LibraryLocation, std::ifstream::in | std::ifstream::binary);
		for(psl::string8_t line; getline(library, line);)
		{
			size_t start		= line.find("UID=") + 4;
			size_t end			= line.find("]", start);
			psl::string8_t meta = line.substr(start, end - start);
			auto uid			= UID::convert(meta);
			size_t startPath	= line.find("METAPATH=") + 9;
			size_t endPath		= line.find("]", startPath);

			psl::string8_t metapath  = line.substr(startPath, endPath - startPath);
			psl::string8_t filepath  = metapath.substr(0, metapath.size() - (META_EXTENSION.size() + 1));
			psl::string8_t extension = filepath.substr(filepath.find_last_of('.') + 1);

			file* metaPtr = nullptr;
			if(!utility::platform::file::exists(root + metapath))
			{
				// todo handle error
				continue;
			}
			s.deserialize<decode_from_format>(metaPtr, root + metapath);

			auto pair = m_MetaData.emplace(metaPtr->ID(), std::move(metaPtr));
			m_TagMap[filepath].insert(pair.first->second.data->ID());
			pair.first->second.flags[0]		= true;
			pair.first->second.readableName = filepath;
		}
		library.close(); // The Library is CLOSED! #NoRuPauligy
	}
}


library::~library() {}

bool library::serialize(const UID& uid)
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || it->second.flags[0] != true) return false;

	psl::string8_t filepath = psl::string8_t(m_LibraryFolder) +
							  psl::to_string8_t(utility::platform::directory::seperator) + it->second.readableName;
	serializer s;
	s.serialize<encode_to_format>(it->second.data.get(), psl::string8::view{filepath});
	return true;
};

bool library::remove(const UID& uid, bool safe_mode)
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || (safe_mode && it->second.referencedBy.size() > 0)) return false;

	// notify all those who are referencing me that I no longer exist
	for(auto& ref : it->second.referencedBy)
	{
		if(auto refIt = m_MetaData.find(ref); refIt != m_MetaData.end())
		{
			refIt->second.referencing.erase(uid);
		}
	}

	// notify all those I'm referencing that I no longer exist
	for(auto& ref : it->second.referencing)
	{
		if(auto refIt = m_MetaData.find(ref); refIt != m_MetaData.end())
		{
			refIt->second.referencedBy.erase(uid);
		}
	}
	// remove all tags
	for(auto& tag : it->second.data->m_Tags.value)
	{
		auto tagIt = m_TagMap.find(tag);
		if(tagIt == m_TagMap.end()) continue;

		tagIt->second.erase(uid);
		if(tagIt->second.size() == 0) m_TagMap.erase(tagIt);
	}

	m_MetaData.erase(it);
	return true;
}

bool library::contains(const UID& uid) const { return m_MetaData.find(uid) != m_MetaData.end(); }


std::optional<UID> library::find(psl::string8::view tag) const
{
	auto it = m_TagMap.find(psl::string8_t(tag));
	if(it == m_TagMap.end() || it->second.size() == 0) return {};

	return *std::begin(it->second);
}
std::unordered_set<UID> library::find_all(psl::string8::view tag) const
{
	auto it = m_TagMap.find(psl::string8_t(tag));
	if(it == m_TagMap.end()) return {};
	return it->second;
}

const std::vector<psl::string8_t>& library::tags(const UID& uid) const
{
	static std::vector<psl::string8_t> empty;
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData)) return empty;

	return it->second.data->m_Tags;
}

bool library::has_tag(const UID& uid, psl::string8::view tag) const
{
	auto it = m_TagMap.find(psl::string8_t(tag));
	if(it == m_TagMap.end()) return false;

	return it->second.find(uid) == it->second.end();
}

bool library::set(const UID& uid, psl::string8::view tag)
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData)) return false;

	m_TagMap[psl::string8_t(tag)].insert(uid);
	it->second.data->m_Tags.value.push_back(psl::string8_t(tag));
	return true;
}
bool library::set(const UID& uid, std::vector<psl::string8::view> tags)
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData)) return false;

	for(auto& tag : tags)
	{
		m_TagMap[psl::string8_t(tag)].insert(uid);
	}
	it->second.data->m_Tags.value.insert(it->second.data->m_Tags.value.end(), tags.begin(), tags.end());
	return true;
}

std::unordered_set<UID> library::referencing(const UID& uid) const
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData)) return {};

	return it->second.referencing;
}
std::unordered_set<UID> library::referencedBy(const UID& uid) const
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData)) return {};

	return it->second.referencedBy;
}

bool library::is_physical_file(const UID& uid) const
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData)) return false;

	return it->second.flags[0] == true;
}
std::optional<psl::string8_t> library::get_physical_location(const UID& uid) const
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || it->second.flags[0] != true) return {};

	return psl::string8_t(m_LibraryFolder) + psl::to_string8_t(utility::platform::directory::seperator) +
		   it->second.readableName;
}

size_t library::size() const { return m_MetaData.size(); }


std::optional<psl::string8::view> library::load(const UID& uid)
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || it->second.flags[0] != true) return {};

	if(auto res = utility::platform::file::read(psl::from_string8_t(m_LibraryFolder) +
												utility::platform::directory::seperator +
												psl::from_string8_t(it->second.readableName));
	   res)
	{
		it->second.file_data = psl::string(res.value().data(), res.value().size());
		return it->second.file_data;
	}
	return {};
}

bool library::unload(const UID& uid)
{
	auto it = m_MetaData.find(uid);
	if(it == std::end(m_MetaData) || it->second.flags[0] != true || it->second.file_data.empty()) return false;

	it->second.file_data = {};
	return true;
}
