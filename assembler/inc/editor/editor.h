#pragma once
#include "psl/memory/region.h"

#include "resource/cache.h"

#include "gfx/types.h"

namespace assembler {
class editor {
  public:
	editor(core::gfx::graphics_backend backend);
	~editor() = default;

	editor(editor const& other)				   = default;
	editor(editor&& other) noexcept			   = default;
	editor& operator=(editor const& other)	   = default;
	editor& operator=(editor&& other) noexcept = default;

	/// \note this does not return until the end of the editor's lifetime
	void run();

  private:
	core::gfx::graphics_backend m_Backend;
	memory::region m_ResourceRegion;
	core::resource::cache m_Cache;
};
}	 // namespace assembler