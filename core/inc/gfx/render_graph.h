#pragma once
#include "systems/resource.h"
#include "unique_ptr.h"
#include "view_ptr.h"
#include "array_view.h"

namespace core::ivk
{
	class context;
}

namespace core::gfx
{
	class pass;
	class framebuffer;
	class swapchain;

	class render_graph
	{
		struct graph_element
		{
			psl::unique_ptr<core::gfx::pass> pass;
			psl::array<psl::view_ptr<core::gfx::pass>> connected_by;
			psl::array<psl::view_ptr<core::gfx::pass>> connects_to;
		};

	  public:
		psl::view_ptr<core::gfx::pass> create_pass(core::resource::handle<core::ivk::context> context,
												   core::resource::handle<core::gfx::swapchain> swapchain);
		psl::view_ptr<core::gfx::pass> create_pass(core::resource::handle<core::ivk::context> context,
												   core::resource::handle<core::gfx::framebuffer> framebuffer);

		bool connect(psl::view_ptr<pass> child, psl::view_ptr<pass> root) noexcept;
		bool disconnect(psl::view_ptr<pass> pass) noexcept;
		bool disconnect(psl::view_ptr<pass> child, psl::view_ptr<pass> root) noexcept;

		bool erase(psl::view_ptr<pass> pass) noexcept;
		void present();

	  private:
		void rebuild() noexcept;
		psl::array<graph_element> m_Passes;
		psl::array<psl::array<graph_element*>> m_Graph;

		bool m_Rebuild{false};
	};
} // namespace core::gfx