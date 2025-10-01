#include "importers/importer.hpp"

#include "stdafx.h"
#include <fmt/ranges.h>
#include <fstream>
#include <ranges>

namespace assembler::importer {

auto operation_t::verify() -> bool {
	return true;
}

auto delete_file_t::verify() -> bool {
	return std::filesystem::exists(path);
}

auto delete_file_t::apply() -> bool {
	assembler::log->info("    Deleting file {}", path.string());
	return std::filesystem::remove(path);
}

auto copy_file_t::verify() -> bool {
	return std::filesystem::exists(from);
}

auto copy_file_t::apply() -> bool {
	if(!std::filesystem::exists(to.parent_path())) {
		std::filesystem::create_directories(to.parent_path());
	}
	std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
	std::filesystem::last_write_time(to, std::filesystem::last_write_time(from));
	assembler::log->info("    Copied file from {} to {}", from.string(), to.string());
	return true;
}

auto move_file_t::verify() -> bool {
	return std::filesystem::exists(from);
}

auto move_file_t::apply() -> bool {
	assembler::log->info("    Moving file from {} to {}", from.string(), to.string());
	std::filesystem::rename(from, to);
	return true;
}

auto write_file_t::apply() -> bool {
	assembler::log->info("    Writing file {}", path.string());

	if(!std::filesystem::exists(path.parent_path())) {
		std::filesystem::create_directories(path.parent_path());
	}
	std::ofstream file(path, std::ios::binary);
	file.write(reinterpret_cast<char*>(data.data()), data.size());
	return true;
}

auto importer_base_t::rebase_to_build_dir(std::filesystem::path const& file) const -> std::filesystem::path {
	auto rel_source_path = std::filesystem::relative(file,
													 std::filesystem::path {project().project_directory()} /
													   std::filesystem::path {project().source_directory()});
	auto build_dir =
	  std::filesystem::path {project().project_directory()} / std::filesystem::path {project().build_directory()};
	return std::filesystem::absolute(build_dir / rel_source_path);
}

auto importer_t::map_extension(psl::string const& extensions,
							   std::shared_ptr<importer_base_t> const& importers) -> bool {
	return map_extension(psl::array<psl::string> {extensions},
						 psl::array<std::shared_ptr<importer_base_t>> {importers});
}
auto importer_t::map_extension(psl::string const& extensions,
							   psl::array<std::shared_ptr<importer_base_t>> const& importers) -> bool {
	return map_extension(psl::array<psl::string> {extensions}, importers);
}
auto importer_t::map_extension(psl::array<psl::string> const& extensions,
							   std::shared_ptr<importer_base_t> const& importers) -> bool {
	return map_extension(extensions, psl::array<std::shared_ptr<importer_base_t>> {importers});
}

auto importer_t::map_extension(psl::array<psl::string> const& extensions,
							   psl::array<std::shared_ptr<importer_base_t>> const& importers) -> bool {
	auto pass = std::all_of(std::begin(importers), std::end(importers), [this](auto const& importer) {
		return std::find_if(std::begin(m_Importers), std::end(m_Importers), [&importer](auto const& entry) {
				   return entry.importer == importer;
			   }) != m_Importers.end();
	});
	if(!pass)
		return false;

	for(auto const& extension : extensions) {
		auto ext_it = m_ExtensionToImporter.find(extension);
		if(ext_it != m_ExtensionToImporter.end()) {
			ext_it->second.insert(std::end(ext_it->second), std::begin(importers), std::end(importers));
		} else {
			m_ExtensionToImporter.emplace(extension, importers);
		}
	}
	return true;
}

auto importer_t::default_importer(std::shared_ptr<importer_base_t> const& importer) -> bool {
	return default_importer(psl::array<std::shared_ptr<importer_base_t>> {importer});
}

auto importer_t::default_importer(psl::array<std::shared_ptr<importer_base_t>> const& importers) -> bool {
	if(!std::all_of(std::begin(importers), std::end(importers), [this](auto const& importer) {
		   return std::find_if(std::begin(m_Importers), std::end(m_Importers), [&importer](auto const& entry) {
					  return entry.importer == importer;
				  }) != m_Importers.end();
	   })) {
		return false;
	}

	m_DefaultImporters.insert(std::end(m_DefaultImporters), std::begin(importers), std::end(importers));
	return true;
}

auto importer_t::unregister_importer(std::shared_ptr<importer_base_t> importer) -> bool {
	auto it = std::find_if(std::begin(m_Importers), std::end(m_Importers), [&importer](auto const& entry) {
		return entry.importer == importer;
	});
	if(it == m_Importers.end())
		return false;

	for(auto& [key, value] : m_ExtensionToImporter) {
		value.erase(std::remove(std::begin(value), std::end(value), importer), std::end(value));
	}
	m_Importers.erase(it);

	m_DefaultImporters.erase(std::remove(std::begin(m_DefaultImporters), std::end(m_DefaultImporters), importer),
							 std::end(m_DefaultImporters));

	// todo: this will be a bit slow with a lot of importers and extensions, but should be fine for now
	for(auto& [key, value] : m_ExtensionToImporter) {
		value.erase(std::remove(std::begin(value), std::end(value), importer), std::end(value));
	}
	return true;
}
auto importer_t::import(std::filesystem::path const& file) -> bool {
	auto extension = file.extension().string();

	if(file.is_relative()) {
		assembler::log->error("The file path is relative, this is not supported. {}", file.string());
		return false;
	}

	if(std::find(std::begin(m_IgnoredExtensions), std::end(m_IgnoredExtensions), extension) !=
	   std::end(m_IgnoredExtensions)) {
		return true;
	}

	auto extension_mapping_it = m_ExtensionToImporter.find(extension);

	auto const& importers =
	  (extension_mapping_it != m_ExtensionToImporter.end()) ? extension_mapping_it->second : m_DefaultImporters;

	if(importers.empty()) {
		return true;
	}

	auto names = importers | std::views::transform([](auto const& obj) { return obj->name(); });
	assembler::log->info("Importing file {} with {}", file.string(), names);
	psl::array<std::unique_ptr<operation_t>> operations {};
	// accumulate the results for all importer operations
	for(auto const& importer : importers) {
		auto result = importer->import(file);
		// stop on first failure
		if(!result) {
			return false;
		}

		auto result_operations = result.consume();
		operations.insert(std::end(operations),
						  std::make_move_iterator(std::begin(result_operations)),
						  std::make_move_iterator(std::end(result_operations)));
	}

	// Verify all operations before applying them, this is to ensure that the
	// operations believe they can actually do their operation. Failing here usually
	// indicates an issue in the importer itself (likely bug).
	if(!std::all_of(
		 std::begin(operations), std::end(operations), [](auto const& operation) { return operation->verify(); })) {
		return false;
	}

	// apply all operations if all importers succeeded
	for(auto& operation : operations) {
		// operations can still fail at this stage, though that would indicate a filesystem issue
		if(!operation->apply()) {
			return false;
		}
	}
	return true;
}

}	 // namespace assembler::importer