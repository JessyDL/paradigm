#pragma once

#include <filesystem>
#include <memory>
#include <numeric>
#include <psl/array.hpp>
#include <unordered_map>

#include "data/project.hpp"
#include "psl/view_ptr.hpp"

namespace assembler::importer {

struct operation_t {
	virtual ~operation_t() = default;

	virtual auto verify() -> bool;
	virtual auto apply() -> bool = 0;
};
struct delete_file_t : public operation_t {
	std::filesystem::path path;

	delete_file_t(std::filesystem::path path) : path(path) {}

	auto verify() -> bool override;
	auto apply() -> bool override;
};

struct move_file_t : public operation_t {
	std::filesystem::path from;
	std::filesystem::path to;

	move_file_t(std::filesystem::path from, std::filesystem::path to) : from(from), to(to) {}

	auto verify() -> bool override;
	auto apply() -> bool override;
};

struct copy_file_t : public operation_t {
	std::filesystem::path from;
	std::filesystem::path to;

	copy_file_t(std::filesystem::path from, std::filesystem::path to) : from(from), to(to) {}

	auto verify() -> bool override;
	auto apply() -> bool override;
};

struct write_file_t : public operation_t {
	std::filesystem::path path;
	psl::array<std::byte> data;

	write_file_t(std::filesystem::path path, psl::array<std::byte> data) : path(path), data(data) {}

	auto apply() -> bool override;
};

class importer_result_t {
  public:
	importer_result_t() = default;
	importer_result_t(bool success) : success(success) {}
	importer_result_t(std::unique_ptr<operation_t> operations) : success(true) {
		this->operations.push_back(std::move(operations));
	}

	auto add(std::unique_ptr<operation_t> operations) -> void {
		this->operations.push_back(std::move(operations));
		success = true;
	}

	operator bool() const {
		return success;
	}
	auto&& consume() {
		return std::move(operations);
	}

	auto size() const {
		return operations.size();
	}
	auto empty() const {
		return operations.empty();
	}

  private:
	psl::array<std::unique_ptr<operation_t>> operations {};
	bool success {true};
};

class importer_t;

class importer_base_t {
	friend class importer_t;

  public:
	importer_base_t()															= default;
	virtual ~importer_base_t()													= default;
	virtual auto import(std::filesystem::path const& file) -> importer_result_t = 0;

	virtual psl::string_view name() const noexcept = 0;

  protected:
	auto project() const -> assembler::data::project_t const& {
		return m_Project.get();
	}

	// helper function to rebase a path to the build directory, it will return a path that is relative to the build
	// based on the path relative to the source directory.
	// it is recommended for importers to use this function to ensure that the paths are correctly rebased.
	[[nodiscard]] auto rebase_to_build_dir(std::filesystem::path const& path) const -> std::filesystem::path;

  private:
	// lifetime is guaranteed by the importer_t
	psl::view_ptr<assembler::data::project_t> m_Project {nullptr};
};

class importer_t {
	struct entry_t {
		entry_t(std::shared_ptr<importer_base_t> importer) : importer(std::move(importer)) {}
		std::shared_ptr<importer_base_t> importer {nullptr};
		psl::array<psl::string> extensions {};
	};

  public:
	importer_t(assembler::data::project_t const& project = {})
		: m_Project(std::make_shared<assembler::data::project_t>(project)) {}

	// register an importer, this will return an id that can be used to map extensions to the importer.
	// the ImporterType should be a class that derives from importer_base_t.
	template <typename ImporterType, typename... Args>
	[[nodiscard("you will need the result to register it for an extension, see `map_extension`")]] auto
	register_importer(Args&&... args) -> std::shared_ptr<ImporterType>
		requires(std::is_base_of_v<importer_base_t, ImporterType>)
	{
		auto shared		  = std::make_shared<ImporterType>(std::forward<Args>(args)...);
		shared->m_Project = m_Project.get();
		m_Importers.push_back({shared});
		return shared;
	}

	auto map_extension(psl::string const& extensions, std::shared_ptr<importer_base_t> const& importers) -> bool;
	auto map_extension(psl::string const& extensions,
					   psl::array<std::shared_ptr<importer_base_t>> const& importers) -> bool;
	auto map_extension(psl::array<psl::string> const& extensions,
					   std::shared_ptr<importer_base_t> const& importers) -> bool;
	auto map_extension(psl::array<psl::string> const& extensions,
					   psl::array<std::shared_ptr<importer_base_t>> const& importers) -> bool;


	auto default_importer(std::shared_ptr<importer_base_t> const& importer) -> bool;
	auto default_importer(psl::array<std::shared_ptr<importer_base_t>> const& importers) -> bool;

	auto unregister_importer(std::shared_ptr<importer_base_t> importer) -> bool;

	auto ignore_extension(psl::string const& extension) -> void {
		m_IgnoredExtensions.push_back(extension);
	}

	// will return true if the file was imported, false otherwise.
	// false can happen in two cases:
	// 1. the file is not supported by any importer
	// 2. the file is supported by an importer, but the importer failed to import the file.
	// in case of 2, the importer should log the error
	// note that on-fail the importers will not apply their operations (if the importer is well behaved and uses the
	// importer_result_t to do its operations on the filesystem), though importers' internal state might be changed.
	// but the operations after all importers have been invoked cannot be rolled back, their failure indicates
	// issues in the filesystem itself.
	[[nodiscard]] auto import(std::filesystem::path const& file) -> bool;

  private:
	psl::array<entry_t> m_Importers {};
	std::unordered_map<psl::string, psl::array<std::shared_ptr<importer_base_t>>> m_ExtensionToImporter {};
	psl::array<std::shared_ptr<importer_base_t>> m_DefaultImporters {};
	std::shared_ptr<assembler::data::project_t> m_Project {nullptr};
	psl::array<psl::string> m_IgnoredExtensions {};
};
}	 // namespace assembler::importer
