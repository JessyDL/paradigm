#pragma once

#include "importers/importer.hpp"
#include <filesystem>

namespace assembler::importer {
class audio_t : public importer_base_t {
  public:
	auto import(std::filesystem::path const& file) -> importer_result_t override;
	psl::string_view name() const noexcept override {
		return "Audio Importer";
	}

  private:
};
}	 // namespace assembler::importer