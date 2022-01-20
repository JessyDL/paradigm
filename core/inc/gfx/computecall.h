#pragma once
#include "fwd/gfx/buffer.h"
#include "fwd/gfx/compute.h"
#include "fwd/gfx/texture.h"
#include "fwd/resource/resource.h"
#include "psl/array.hpp"
#include "psl/static_array.hpp"
#include "resource/handle.h"

namespace core::gfx
{
	class compute;

	class computecall
	{
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