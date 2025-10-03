#pragma once
#include "cli/value.hpp"
#include "core/meta/shader.hpp"
#include "core/meta/texture.hpp"
#include "psl/array_view.hpp"
#include "psl/library.hpp"
#include "psl/meta.hpp"
#include "psl/terminal_utils.hpp"
#include "utils.hpp"
#include <cstdint>
#include <filesystem>

#include "data/project.hpp"
#include "details/texture_utils.hpp"


namespace assembler::generators {
class meta {
	template <typename T>
	using cli_value = psl::cli::value<T>;

	using cli_pack = psl::cli::pack;

	struct extension {
	  public:
		template <typename S>
		void serialize(S& s) {
			s << meta << extensions;
		}

		static constexpr char const serialization_name[6] {"ENTRY"};
		psl::serialization::property<"EXTENSIONS", psl::array<psl::string>> extensions;
		psl::serialization::property<"META", psl::string> meta;
	};
	struct environment {
	  public:
		template <typename S>
		void serialize(S& s) {
			s << name << extensions;
		}

		static constexpr char const serialization_name[12] {"ENVIRONMENT"};
		psl::serialization::property<"EXTENSIONS", psl::array<psl::string>> extensions;
		psl::serialization::property<"NAME", psl::string> name;
	};
	struct mapping_table {
	  public:
		template <typename S>
		void serialize(S& s) {
			s << m_Mappings << m_Environments;
		}
		static constexpr char const serialization_name[6] {"TABLE"};

		std::unordered_map<psl::string, psl::string> mappings() const noexcept {
			std::unordered_map<psl::string, psl::string> res;
			for(auto const& entry : m_Mappings.value) {
				for(auto const& ext : entry.extensions.value) {
					res.emplace(ext, entry.meta.value);
				}
			}
			return res;
		}

		std::unordered_map<psl::string, psl::array<psl::string>> environments() const noexcept {
			std::unordered_map<psl::string, psl::array<psl::string>> res;
			for(auto const& entry : m_Environments.value) {
				for(auto const& ext : entry.extensions.value) {
					res[ext].emplace_back(entry.name.value);
				}
			}
			return res;
		}

	  private:
		psl::serialization::property<"MAPPING", psl::array<extension>> m_Mappings;
		psl::serialization::property<"ENVIRONMENTS", psl::array<environment>> m_Environments;
	};

  public:
	meta();
	cli_pack meta_pack();
	cli_pack library_pack();

  private:
	void on_library_generate(cli_pack& pack) {
		// --generate -l -d "C:\Projects\github\example_data\library" -r "C:\Projects\github\example_data\data"
		auto lib_dir  = pack["directory"]->as<psl::string>().get();
		auto lib_name = pack["name"]->as<psl::string>().get();
		auto res_dir  = pack["resource"]->as<psl::string>().get();
		auto clean	  = pack["clean"]->as<bool>().get();

		generate_library(lib_dir, lib_name, res_dir, clean);
	}

  public:
	void generate_library(psl::string lib_dir, psl::string lib_name, psl::string res_dir, bool clean);
	void generate_meta(psl::string in_path,
					   psl::string out_path,
					   psl::string meta_type,
					   bool recursive_search,
					   bool force_regenerate,
					   bool update);

  private:
	void on_meta_generate(cli_pack& pack);

	std::unordered_map<psl::string, psl::string> m_FileMaps;
	std::unordered_map<psl::string, psl::array<psl::string>> m_EnvMaps;
};
}	 // namespace assembler::generators
// const uint64_t core::meta::texture::polymorphic_identity{serialization::register_polymorphic<core::meta::texture>()};
// const uint64_t core::meta::shader::polymorphic_identity{serialization::register_polymorphic<core::meta::shader>()};
