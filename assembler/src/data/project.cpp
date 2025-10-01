#include "data/project.hpp"
#include "stdafx.h"

namespace assembler::data {
void project_t::version_check() const noexcept {
	if(m_Version != CURRENT_VERSION) {
		assembler::log->warn(
		  "Project version mismatch: expected {}, got {}. "
		  "Please update your project file. We will continue trying to load, but issues might arise, not all "
		  "versions are backwards compatible.",
		  CURRENT_VERSION,
		  m_Version.value);
	}
}
}	 // namespace assembler::data