#pragma once
#include <vector>
#include "gfx/bundle.h"
#include "vk/geometry.h"
#include "fwd/systems/resource.h"

namespace core::ivk
{
	class geometry;
}

namespace core::gfx
{
	class bundle;

	class drawcall
	{
		friend class drawgroup;

	  public:
		drawcall(core::resource::handle<core::gfx::bundle> bundle,
				 const std::vector<core::resource::handle<core::ivk::geometry>>& geometry = {}) noexcept;
		~drawcall()				  = default;
		drawcall(const drawcall&) = default;
		drawcall(drawcall&&)	  = default;
		drawcall& operator=(const drawcall&) = delete;
		drawcall& operator=(drawcall&&) = delete;

		bool add(core::resource::handle<core::ivk::geometry> geometry) noexcept;
		bool remove(core::resource::handle<core::ivk::geometry> geometry) noexcept;
		bool remove(const psl::UID& geometry) noexcept;

		void bundle(core::resource::handle<core::gfx::bundle> bundle) noexcept;
		core::resource::handle<core::gfx::bundle> bundle() const noexcept;

	  private:
		core::resource::handle<core::gfx::bundle> m_Bundle;
		std::vector<std::pair<core::resource::handle<core::ivk::geometry>, size_t>> m_Geometry;
	};
} // namespace core::gfx