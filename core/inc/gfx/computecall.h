#pragma once
#include "fwd/resource/resource.h"
#include "resource/handle.h"
#include "fwd/gfx/texture.h"
#include "fwd/gfx/buffer.h"
#include "fwd/gfx/compute.h"
#include "psl/static_array.h"
#include "psl/array.h"

namespace core::gfx
{
	class compute;

	class computecall
	{
	  public:
		computecall(core::resource::handle<compute> compute);
		/*computecall(core::resource::handle<compute> compute,
					psl::array<core::resource::handle<core::gfx::buffer>> buffers);
		computecall(core::resource::handle<compute> compute,
					psl::array<core::resource::handle<core::gfx::texture>> textures);
		computecall(core::resource::handle<compute> compute,
					psl::array<core::resource::handle<core::gfx::texture>> textures,
					psl::array<core::resource::handle<core::gfx::buffer>> buffers);*/

		void dispatch();
	  private:
		psl::static_array<uint32_t, 3> m_DispatchSize{512, 512, 1};
		core::resource::handle<core::gfx::compute> m_Compute;
	};
} // namespace core::gfx