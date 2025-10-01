#pragma once

#include "importers/importer.hpp"
#include <filesystem>

namespace assembler::importer {

class meta_t : public importer_base_t {
  public:
	meta_t(bool force_regenerate = false) : force_regenerate(force_regenerate), importer_base_t() {}
	auto import(std::filesystem::path const& file) -> importer_result_t override;

	psl::string_view name() const noexcept override {
		return "Meta Importer";
	}

  private:
	bool force_regenerate {false};
};
}	 // namespace assembler::importer