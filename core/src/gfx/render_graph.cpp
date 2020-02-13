#include "gfx/render_graph.h"
#include "gfx/drawpass.h"
#include "gfx/computepass.h"
#include "gfx/context.h"
#include "gfx/swapchain.h"
#include "gfx/framebuffer.h"

using namespace core::gfx;
using core::resource::handle;

render_graph::~render_graph(){};

psl::view_ptr<core::gfx::drawpass> render_graph::create_drawpass(handle<core::gfx::context> context,
																 handle<core::gfx::swapchain> swapchain)
{
	auto& element = m_Passes.emplace_back();
	auto pass	  = new drawpass(context, swapchain);
	element.pass  = pass;

	return {pass};
}
psl::view_ptr<core::gfx::drawpass> render_graph::create_drawpass(handle<core::gfx::context> context,
																 handle<core::gfx::framebuffer> framebuffer)
{
	auto& element = m_Passes.emplace_back();
	auto pass	  = new drawpass(context, framebuffer);
	element.pass  = pass;

	return {pass};
}

psl::view_ptr<core::gfx::computepass> render_graph::create_computepass(handle<core::gfx::context> context) noexcept
{
	auto& element = m_Passes.emplace_back();
	auto pass	  = new computepass(context);
	element.pass  = pass;

	return {pass};
}

template <typename... Ts>
auto get_var_ptr(const std::variant<Ts...>& vars)
{
	return std::visit(utility::templates::overloaded{[](auto&& pass) -> void* { return (void*)&pass.get(); }}, vars);
}

void render_graph::rebuild() noexcept { m_Rebuild = true; }

void render_graph::present()
{
	// todo: we have a failed assumption here till the implementation is done;
	// currently we assume that the "final" pass will be the swapchain
	auto it_final = std::find_if(std::begin(m_Passes), std::end(m_Passes), [](const graph_element& element) {
		return std::visit(utility::templates::overloaded{
							  [](const psl::unique_ptr<core::gfx::drawpass>& pass) { return pass->is_swapchain(); },
							  [](const auto& pass) { return false; }},
						  element.pass);
	});

	if(std::distance(std::begin(m_Passes), it_final) != m_Passes.size() - 1)
		std::iter_swap(it_final, std::prev(std::end(m_Passes)));

	for(auto& node : m_Passes)
	{
		std::visit(utility::templates::overloaded{[rebuild = m_Rebuild](auto&& pass) {
					   pass->prepare();
					   pass->build();
				   }},
				   node.pass);
	}
	m_Rebuild = false;

	for(auto& node : m_Passes)
	{
		std::visit(utility::templates::overloaded{[](auto&& pass) { pass->present(); }}, node.pass);
	}
}

bool render_graph::connect(render_graph::view_var_t child, render_graph::view_var_t root) noexcept
{
	auto it_root = std::find_if(std::begin(m_Passes), std::end(m_Passes), [&root](const graph_element& element) {
		return get_var_ptr(element.pass) == get_var_ptr(root);
	});

	auto it_child = std::find_if(std::begin(m_Passes), std::end(m_Passes), [&child](const graph_element& element) {
		return get_var_ptr(element.pass) == get_var_ptr(child);
	});

	if(it_root != std::end(m_Passes) && it_child != std::end(m_Passes))
	{
		it_root->connected_by.emplace_back(child);
		std::visit(utility::templates::overloaded{[&it_child](auto& pass) {
					   std::visit(utility::templates::overloaded{[&pass](auto& child_pass) {
									  pass->connect(psl::view_ptr{&child_pass.get()});
								  }},
								  it_child->pass);
				   }},
				   it_root->pass);

		std::visit(utility::templates::overloaded{[&it_child](auto& pass) {
					   it_child->connects_to.emplace_back(&pass.get());
				   }},
				   it_root->pass);
		// it_child->connects_to.emplace_back( it_root->pass);
		m_Rebuild = true;
		return true;
	}
	return false;
}

bool render_graph::disconnect(render_graph::view_var_t child, render_graph::view_var_t root) noexcept
{
	auto it_root = std::find_if(std::begin(m_Passes), std::end(m_Passes), [&root](const graph_element& element) {
		return get_var_ptr(element.pass) == get_var_ptr(root);
	});

	auto it_child = std::find_if(std::begin(m_Passes), std::end(m_Passes), [&child](const graph_element& element) {
		return get_var_ptr(element.pass) == get_var_ptr(child);
	});


	if(it_root != std::end(m_Passes) && it_child != std::end(m_Passes))
	{
		it_root->connected_by.erase(
			std::find(std::begin(it_root->connected_by), std::end(it_root->connected_by), child),
			std::end(it_root->connected_by));

		std::visit(utility::templates::overloaded{[&it_child](auto& pass) {
					   std::visit(utility::templates::overloaded{[&pass](auto& child_pass) {
									  pass->disconnect(psl::view_ptr{&child_pass.get()});
								  }},
								  it_child->pass);
				   }},
				   it_root->pass);
		// std::visit(utility::templates::overloaded{ [&it_child](auto& pass) { pass->disconnect(it_child->pass); } },
		// it_root->pass);
		it_child->connects_to.erase(std::find(std::begin(it_root->connects_to), std::end(it_root->connects_to), root),
									std::end(it_root->connects_to));
		m_Rebuild = true;
		return true;
	}
	return false;
}

bool render_graph::disconnect(render_graph::view_var_t child) noexcept
{
	if(auto it_child = std::find_if(
		   std::begin(m_Passes), std::end(m_Passes),
		   [&child](const graph_element& element) { return get_var_ptr(element.pass) == get_var_ptr(child); });
	   it_child != std::end(m_Passes))
	{
		for(auto& pass : it_child->connects_to)
		{
			disconnect(child, pass);
		}
		m_Rebuild = true;
		return true;
	}
	return false;
}

bool render_graph::erase(render_graph::view_var_t pass) noexcept
{
	if(auto it = std::find_if(
		   std::begin(m_Passes), std::end(m_Passes),
		   [&pass](const graph_element& element) { return get_var_ptr(element.pass) == get_var_ptr(pass); });
	   it != std::end(m_Passes))
	{
		disconnect(pass);
		m_Passes.erase(it);
		m_Rebuild = true;
		return true;
	}
	return false;
}