#pragma once
#include <vector>
#include "gfx/bundle.h"
#include "fwd/resource/resource.h"

#ifdef PE_VULKAN
namespace core::ivk
{
	class pass;
}
#endif
#ifdef PE_GLES
namespace core::igles
{
	class pass;
}
#endif
namespace core::gfx
{
	class bundle;
	class geometry;

	class drawcall
	{
		friend class drawgroup;
#ifdef PE_VULKAN
		friend class core::ivk::pass;
#endif
#ifdef PE_GLES
		friend class core::igles::pass;
#endif

	  public:
		drawcall(core::resource::handle<core::gfx::bundle> bundle,
				 const std::vector<core::resource::handle<core::gfx::geometry>>& geometry = {}) noexcept;
		~drawcall()				  = default;
		drawcall(const drawcall&) = default;
		drawcall(drawcall&&)	  = default;
		drawcall& operator=(const drawcall&) = delete;
		drawcall& operator=(drawcall&&) = delete;

		bool add(core::resource::handle<core::gfx::geometry> geometry) noexcept;
		bool remove(core::resource::handle<core::gfx::geometry> geometry) noexcept;
		bool remove(const psl::UID& geometry) noexcept;

		void bundle(core::resource::handle<core::gfx::bundle> bundle) noexcept;
		core::resource::handle<core::gfx::bundle> bundle() const noexcept;

	  private:
		core::resource::handle<core::gfx::bundle> m_Bundle;
		std::vector<std::pair<core::resource::handle<core::gfx::geometry>, size_t>> m_Geometry;
	};
} // namespace core::gfx