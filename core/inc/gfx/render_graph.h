#pragma once
#include "resource/resource.hpp"
#include "psl/unique_ptr.h"
#include "psl/view_ptr.h"
#include "psl/array_view.h"
#include <variant>

namespace core::gfx
{
	class context;
	class framebuffer;
	class swapchain;
	class drawpass;
	class computepass;
} // namespace core::gfx

namespace core::gfx
{
	class render_graph
	{
	  public:
		using view_var_t = std::variant<psl::view_ptr<core::gfx::drawpass>, psl::view_ptr<core::gfx::computepass>>;
		using unique_var_t =
			std::variant<psl::unique_ptr<core::gfx::drawpass>, psl::unique_ptr<core::gfx::computepass>>;

	  private:
		struct graph_element
		{
			unique_var_t pass;
			psl::array<view_var_t> connected_by;
			psl::array<view_var_t> connects_to;
		};

	  public:
		  ~render_graph();

		psl::view_ptr<core::gfx::drawpass> create_drawpass(core::resource::handle<core::gfx::context> context,
													   core::resource::handle<core::gfx::swapchain> swapchain);
		psl::view_ptr<core::gfx::drawpass> create_drawpass(core::resource::handle<core::gfx::context> context,
													   core::resource::handle<core::gfx::framebuffer> framebuffer);
		psl::view_ptr<core::gfx::computepass> create_computepass(core::resource::handle<core::gfx::context> context) noexcept;
		bool connect(view_var_t child, view_var_t root) noexcept;
		bool disconnect(view_var_t pass) noexcept;
		bool disconnect(view_var_t child, view_var_t root) noexcept;

		bool erase(view_var_t pass) noexcept;
		void present();

	  private:
		void rebuild() noexcept;
		psl::array<graph_element> m_Passes;
		psl::array<psl::array<graph_element*>> m_Graph;

		bool m_Rebuild{true};
	};
} // namespace core::gfx