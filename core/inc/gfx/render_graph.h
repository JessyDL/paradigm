#pragma once
#include "systems/resource.h"
#include "unique_ptr.h"

namespace core::gfx
{
	class pass;
	class context;
	class framebuffer;
	class swapchain;

	class render_graph
	{
		struct graph_element
		{
			psl::unique_ptr<core::gfx::pass> pass;
			psl::array<core::gfx::pass*> dependencies;
		};
	public:
		core::gfx::pass& create_pass(core::resource::handle<core::gfx::context> context, core::resource::handle<core::gfx::swapchain> swapchain);
		core::gfx::pass& create_pass(core::resource::handle<core::gfx::context> context, core::resource::handle<core::gfx::framebuffer> framebuffer);

		bool add_dependency(core::gfx::pass& root, core::gfx::pass& child) noexcept;

		void present();
	private:
		psl::array<graph_element> m_Passes;
		psl::array<psl::array<graph_element*>> m_Graph;
	};
}