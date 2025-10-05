#include "details/shader_cache.hpp"
#include "stdafx.hpp"
#include <chrono>

namespace assembler::details {
auto shader_cache_t::get_transformed_content(entry_t const& entry) -> std::optional<psl::string> {
	psl::array<entry_key_t> excludes {};
	return get_transformed_content(entry, excludes);
}
auto shader_cache_t::get_transformed_content(entry_t const& entry,
											 psl::array<entry_key_t>& excludes) -> std::optional<psl::string> {
	psl::string content = entry.content;
	size_t offset		= 0;
	for(auto const& include : entry.includes) {
		if(std::find(std::begin(excludes), std::end(excludes), include.include) != std::end(excludes)) {
			continue;
		}
		auto include_entry = get_entry(include.include);
		if(!include_entry) {
			assembler::log->error("error processing '{}': could not find the include '{}'.",
								  entry.path.string(),
								  include.include.path.string());
			return std::nullopt;
		}
		excludes.push_back(include.include);
		auto include_content = get_transformed_content(include_entry.value(), excludes);
		if(!include_content) {
			assembler::log->error("error processing '{}': could not find the include '{}'.",
								  entry.path.string(),
								  include.include.path.string());
			return std::nullopt;
		}
		content.insert(include.index + offset, include_content.value());
		offset += include_content.value().size();
	}
	return content;
}

auto shader_cache_t::get_entry(std::filesystem::path const& key) -> std::optional<entry_t> {
	auto absolute_path = std::filesystem::absolute(key);
	auto entry		   = entry_t {absolute_path};
	auto it			   = m_Entries.find(entry);
	if(it == m_Entries.end()) {
		auto data	  = psl::utility::platform::file::read(absolute_path.string()).value_or("");
		entry.content = data;
		entry.last_modified =
		  std::chrono::clock_cast<std::chrono::system_clock>(std::filesystem::last_write_time(absolute_path));

		if(!parse_includes(entry))
			return std::nullopt;

		m_Entries.insert(entry);
		return entry;
	}
	return *it;
}

auto shader_cache_t::parse_includes(entry_t& entry) -> bool {
	auto pos_include = entry.content.find("#include");

	while(pos_include != psl::string::npos) {
		auto pos_next_token = entry.content.find_first_of("\"'`\n", pos_include);
		if(pos_next_token == psl::string::npos) {
			auto line_number = std::count(entry.content.begin(), entry.content.begin() + pos_include, '\n');
			assembler::log->error(
			  "error processing '{}': could not deduce the #include path at line {}, unexpected end of file",
			  entry.path.string(),
			  line_number);
			return false;
		} else if(entry.content[pos_next_token] == '\n') {
			auto line_number = std::count(entry.content.begin(), entry.content.begin() + pos_include, '\n');
			assembler::log->error(
			  "error processing '{}': could not deduce the #include path at line {}, unexpected newline (expected "
			  "either '\"', ''', or '`')",
			  entry.path.string(),
			  line_number);
			return false;
		}

		auto include_token = entry.content[pos_next_token];

		auto include_start = pos_next_token + 1;
		// we don't allow for multi-line includes, so we include it in the search so we can break if it's not found
		auto include_end = entry.content.find(include_token, include_start);
		auto end_of_line = entry.content.find('\n', include_start);
		if(include_end == psl::string::npos) {
			auto line_number = std::count(entry.content.begin(), entry.content.begin() + pos_include, '\n');
			assembler::log->error(
			  "error processing '{}': could not deduce the #include path at line {}, unexpected end of file",
			  entry.path.string(),
			  line_number);
			return false;
		} else if(end_of_line <= include_end) {
			auto line_number = std::count(entry.content.begin(), entry.content.begin() + pos_include, '\n');
			assembler::log->error(
			  "error processing '{}': could not deduce the #include path at line {}, unexpected end of include "
			  "(expected '{}'). note that multi-line includes are not supported.",
			  entry.path.string(),
			  line_number,
			  include_token);
			return false;
		}

		auto include_path = std::filesystem::absolute(
		  entry.path.parent_path() /
		  std::filesystem::path(entry.content.substr(include_start, include_end - include_start)));

		auto include_entry = get_entry(include_path);
		if(!include_entry) {
			auto line_number = std::count(entry.content.begin(), entry.content.begin() + pos_include, '\n');
			assembler::log->error("error processing '{}': could not find the include '{}' at line {}.",
								  entry.path.string(),
								  include_path.string(),
								  line_number);
			return false;
		}
		entry.content.erase(pos_include, (include_end + 1) - pos_include);
		entry.includes.push_back({include_path, pos_include});

		pos_include = entry.content.find("#include", pos_include);
	}

	return true;
}

auto shader_cache_t::get(std::filesystem::path const& path) -> std::optional<psl::string> {
	auto absolute_path = std::filesystem::absolute(path);
	auto key		   = entry_t {absolute_path};
	auto it			   = m_Entries.find(key);
	if(it == m_Entries.end()) {
		auto entry	  = entry_t {absolute_path};
		auto data	  = psl::utility::platform::file::read(absolute_path.string()).value_or("");
		entry.content = data;
		entry.last_modified =
		  std::chrono::clock_cast<std::chrono::system_clock>(std::filesystem::last_write_time(absolute_path));

		if(!parse_includes(entry))
			return {};

		m_Entries.insert(entry);
		return get_transformed_content(entry);
	}
	return get_transformed_content(*it);
}
}	 // namespace assembler::details
