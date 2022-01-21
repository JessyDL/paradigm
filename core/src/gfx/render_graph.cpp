#include "gfx/render_graph.hpp"
#include "gfx/computepass.hpp"
#include "gfx/context.hpp"
#include "gfx/drawpass.hpp"
#include "gfx/framebuffer.hpp"
#include "gfx/swapchain.hpp"

using namespace core::gfx;
using core::resource::handle;

psl::view_ptr<core::gfx::drawpass> render_graph::create_drawpass(handle<core::gfx::context> context,
																 handle<core::gfx::swapchain> swapchain)
{
	auto pass = new drawpass(context, swapchain);
	m_RenderGraph.emplace(pass);

	return {pass};
}
psl::view_ptr<core::gfx::drawpass> render_graph::create_drawpass(handle<core::gfx::context> context,
																 handle<core::gfx::framebuffer_t> framebuffer)
{
	auto pass = new drawpass(context, framebuffer);
	m_RenderGraph.emplace(pass);

	return {pass};
}

psl::view_ptr<core::gfx::computepass> render_graph::create_computepass(handle<core::gfx::context> context) noexcept
{
	auto pass = new computepass(context);
	m_RenderGraph.emplace(pass);

	return {pass};
}

template <typename... Ts>
auto get_var_ptr(const std::variant<Ts...>& vars)
{
	return std::visit(utility::templates::overloaded {[](auto&& pass) -> void* { return (void*)&pass.get(); }}, vars);
}

void render_graph::rebuild() noexcept { m_Rebuild = true; }

void render_graph::present()
{
	if(m_Rebuild)
	{
		core::profiler.scope_begin("rebuild");
		m_FlattenedRenderGraph = m_RenderGraph.to_array();
		m_Rebuild			   = false;
		core::profiler.scope_end();
	}

	core::profiler.scope_begin("prepare/build");
	for(auto* ptr : m_FlattenedRenderGraph)
	{
		auto& pass = *ptr;
		std::visit(utility::templates::overloaded {[rebuild = m_Rebuild](auto&& pass) {
					   pass->prepare();
					   pass->build();
				   }},
				   pass);
	}
	core::profiler.scope_end();
	core::profiler.scope_begin("present");
	for(auto* ptr : m_FlattenedRenderGraph)
	{
		auto& pass = *ptr;
		std::visit(utility::templates::overloaded {[](auto&& pass) { pass->present(); }}, pass);
	}
	core::profiler.scope_end();
}

bool render_graph::connect(render_graph::view_var_t child, render_graph::view_var_t root) noexcept
{
	auto child_ptr = m_RenderGraph.find_if(
	  child, [](const auto& lhs, const auto& rhs) { return get_var_ptr(lhs) == get_var_ptr(rhs); });
	auto root_ptr = m_RenderGraph.find_if(
	  root, [](const auto& lhs, const auto& rhs) { return get_var_ptr(lhs) == get_var_ptr(rhs); });

	if(m_RenderGraph.connect(child_ptr, root_ptr))
	{
		std::visit(utility::templates::overloaded {[&root_ptr](auto&& child) {
					   std::visit(
						 utility::templates::overloaded {[&child](auto&& root) { root->connect(&child.get()); }},
						 *root_ptr);
				   }},
				   *child_ptr);
		m_Rebuild = true;
		return true;
	}
	return false;
}

bool render_graph::disconnect(render_graph::view_var_t child, render_graph::view_var_t root) noexcept
{
	auto child_ptr = m_RenderGraph.find_if(
	  child, [](const auto& lhs, const auto& rhs) { return get_var_ptr(lhs) == get_var_ptr(rhs); });
	auto root_ptr = m_RenderGraph.find_if(
	  root, [](const auto& lhs, const auto& rhs) { return get_var_ptr(lhs) == get_var_ptr(rhs); });

	if(m_RenderGraph.disconnect(child_ptr, root_ptr))
	{
		std::visit(utility::templates::overloaded {[&root_ptr](auto&& child) {
					   std::visit(
						 utility::templates::overloaded {[&child](auto&& root) { root->disconnect(&child.get()); }},
						 *root_ptr);
				   }},
				   *child_ptr);
		m_Rebuild = true;
		return true;
	}
	return false;
}

bool render_graph::disconnect(render_graph::view_var_t child) noexcept
{
	auto child_ptr = m_RenderGraph.find_if(
	  child, [](const auto& lhs, const auto& rhs) { return get_var_ptr(lhs) == get_var_ptr(rhs); });
	if(m_RenderGraph.disconnect(child_ptr))
	{
		m_Rebuild = true;
		return true;
	}
	return false;
}

bool render_graph::erase(render_graph::view_var_t pass) noexcept
{
	auto child_ptr = m_RenderGraph.find_if(
	  pass, [](const auto& lhs, const auto& rhs) { return get_var_ptr(lhs) == get_var_ptr(rhs); });
	if(m_RenderGraph.erase(child_ptr))
	{
		m_Rebuild = true;
		return true;
	}
	return false;
}