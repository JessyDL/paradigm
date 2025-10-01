#pragma once
#include "cli/value.h"
#include "psl/ustring.hpp"
#include <filesystem>

#include "psl/serialization/property.hpp"
#include "psl/serialization/serializer.hpp"

namespace assembler::data {
class project_t;
}	 // namespace assembler::data

namespace assembler::generators {
class project {
	template <typename T>
	using cli_value = psl::cli::value<T>;

  public:
	project() = default;

	auto pack() -> psl::cli::pack {
		return psl::cli::pack {
		  std::bind(&project::on_generate, this, std::placeholders::_1),
		  cli_value<psl::string> {"input", "The project file to use", {"input", "i"}, "", false},
		  cli_value<psl::string> {
			"output",
			"The target output directory (defaults to the data directory where the project file is)",
			{"output", "o"},
			"",
			true},
		  cli_value<bool> {"audio",
						   "Explicitly set the importer to import audio, disables other importers by default",
						   {"audio"},
						   false,
						   true},
		  cli_value<bool> {"models",
						   "Explicitly set the importer to import models, disables other importers by default",
						   {"models"},
						   false,
						   true},
		  cli_value<bool> {"shaders",
						   "Explicitly set the importer to import shaders, disables other importers by default",
						   {"shaders"},
						   false,
						   true},
		  cli_value<bool> {"force",
						   "Force re-import of all assets, even if the output file is newer than the source file",
						   {"force", "f"},
						   false},
		};
	}

  private:
	void generate_resource_library(std::filesystem::path path, assembler::data::project_t const& project);
	void on_generate(psl::cli::pack& pack);
	psl::string m_ProjectFile {};
};
}	 // namespace assembler::generators
