#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <unordered_set>

#include "psl/array.hpp"
#include "psl/ustring.hpp"

namespace assembler::details {
class shader_cache_t {
	struct entry_key_t {
		std::filesystem::path path;

		operator std::filesystem::path() const {
			return path;
		}
		bool operator==(entry_key_t const& rhs) const {
			return path == rhs.path;
		}
	};
	struct entry_t : public entry_key_t {
		struct include_t {
			entry_key_t include;
			size_t index;

			bool operator==(include_t const& rhs) const {
				return include.path == rhs.include.path;
			}
			bool operator==(entry_key_t const& rhs) const {
				return include.path == rhs.path;
			}
		};
		std::chrono::time_point<std::chrono::system_clock> last_modified;

		psl::string content;
		psl::array<include_t> includes {};
	};

	struct hash_entry_t {
		size_t operator()(entry_key_t const& entry) const {
			return std::hash<std::filesystem::path> {}(entry.path);
		}
	};

	auto get_transformed_content(entry_t const& entry) -> std::optional<psl::string>;
	auto get_transformed_content(entry_t const& entry, psl::array<entry_key_t>& excludes) -> std::optional<psl::string>;
	auto get_entry(std::filesystem::path const& key) -> std::optional<entry_t>;
	auto parse_includes(entry_t& entry) -> bool;

  public:
	auto get(std::filesystem::path const& path) -> std::optional<psl::string>;

  private:
	std::unordered_set<entry_t, hash_entry_t> m_Entries;
};
}	 // namespace assembler::details