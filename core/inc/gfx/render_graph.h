#pragma once
#include "resource/resource.hpp"
#include "unique_ptr.h"
#include "view_ptr.h"
#include "array_view.h"

namespace core::ivk
{
	class context;
	class framebuffer;
	class swapchain;
	class pass;
}

namespace core::gfx
{

	class render_graph
	{
		struct graph_element
		{
			psl::unique_ptr<core::ivk::pass> pass;
			psl::array<psl::view_ptr<core::ivk::pass>> connected_by;
			psl::array<psl::view_ptr<core::ivk::pass>> connects_to;
		};

	  public:
		psl::view_ptr<core::ivk::pass> create_pass(core::resource::handle<core::ivk::context> context,
												   core::resource::handle<core::ivk::swapchain> swapchain);
		psl::view_ptr<core::ivk::pass> create_pass(core::resource::handle<core::ivk::context> context,
												   core::resource::handle<core::ivk::framebuffer> framebuffer);

		bool connect(psl::view_ptr<core::ivk::pass> child, psl::view_ptr<core::ivk::pass> root) noexcept;
		bool disconnect(psl::view_ptr<core::ivk::pass> pass) noexcept;
		bool disconnect(psl::view_ptr<core::ivk::pass> child, psl::view_ptr<core::ivk::pass> root) noexcept;

		bool erase(psl::view_ptr<core::ivk::pass> pass) noexcept;
		void present();

	  private:
		void rebuild() noexcept;
		psl::array<graph_element> m_Passes;
		psl::array<psl::array<graph_element*>> m_Graph;

		bool m_Rebuild{false};
	};
} // namespace core::gfx