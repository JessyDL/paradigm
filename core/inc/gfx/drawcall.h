#pragma once
#include <vector>
#include "gfx/material.h"
#include "vk/geometry.h"

namespace core::resource
{
	template<typename T>
	class handle;
}


namespace psl
{
	struct UID;
}

namespace core::gfx
{
	class geometry;
	class material;

	class drawcall
	{
		friend class drawgroup;

	  public:
		drawcall(core::resource::handle<core::gfx::material> material,
				 const std::vector<core::resource::handle<core::gfx::geometry>>& geometry = {}) noexcept;
		~drawcall()				  = default;
		drawcall(const drawcall&) = default;
		drawcall(drawcall&&)	  = default;
		drawcall& operator=(const drawcall&) = delete;
		drawcall& operator=(drawcall&&) = delete;

		bool add(core::resource::handle<core::gfx::geometry> geometry) noexcept;
		bool remove(core::resource::handle<core::gfx::geometry> geometry) noexcept;
		bool remove(const psl::UID& geometry) noexcept;

		void material(core::resource::handle<core::gfx::material> material) noexcept;
		core::resource::handle<core::gfx::material> material() const noexcept;
	  private:
		core::resource::handle<core::gfx::material> m_Material;
		std::vector<core::resource::handle<core::gfx::geometry>> m_Geometry;
	};
}