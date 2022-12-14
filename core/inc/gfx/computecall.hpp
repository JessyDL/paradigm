#pragma once
#include "fwd/gfx/buffer.hpp"
#include "fwd/gfx/compute.hpp"
#include "fwd/gfx/texture.hpp"
#include "fwd/resource/resource.hpp"
#include "psl/array.hpp"
#include "psl/static_array.hpp"
#include "resource/handle.hpp"

namespace core::gfx {
class compute;

class computecall {
  public:
	computecall(core::resource::handle<compute> compute);
	/*computecall(core::resource::handle<compute> compute,
				psl::array<core::resource::handle<core::gfx::buffer_t>> buffers);
	computecall(core::resource::handle<compute> compute,
				psl::array<core::resource::handle<core::gfx::texture_t>> textures);
	computecall(core::resource::handle<compute> compute,
				psl::array<core::resource::handle<core::gfx::texture_t>> textures,
				psl::array<core::resource::handle<core::gfx::buffer_t>> buffers);*/

	void dispatch();

  private:
	psl::static_array<uint32_t, 3> m_DispatchSize {512, 512, 1};
	core::resource::handle<core::gfx::compute> m_Compute;
};
}	 // namespace core::gfx
