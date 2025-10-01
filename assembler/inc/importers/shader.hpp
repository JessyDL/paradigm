#pragma once
#include "details/shader_cache.hpp"
#include "importers/importer.hpp"

namespace assembler::importer {
class shader_t : public importer_base_t {
  public:
	auto import(std::filesystem::path const& file) -> importer_result_t override;

	psl::string_view name() const noexcept override {
		return "Shader Importer";
	}

  private:
	bool m_Optimize {false};
	size_t m_GlesVersion {310};
	details::shader_cache_t m_ShaderCache;
};
}	 // namespace assembler::importer