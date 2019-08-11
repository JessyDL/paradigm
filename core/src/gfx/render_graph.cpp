#include "gfx/render_graph.h"
#include "gfx/pass.h"
#include "gfx/context.h"
#include "gfx/swapchain.h"
#include "gfx/framebuffer.h"

using namespace core::gfx;
using core::resource::handle;


psl::view_ptr<pass> render_graph::create_pass(handle<core::gfx::context> context,
											  handle<core::gfx::swapchain> swapchain)
{
	auto& element = m_Passes.emplace_back();
	element.pass  = new pass(context, swapchain);

	return psl::view_ptr<pass>{element.pass};
}
psl::view_ptr<pass> render_graph::create_pass(handle<core::gfx::context> context,
											  handle<core::gfx::framebuffer> framebuffer)
{
	auto& element = m_Passes.emplace_back();
	element.pass  = new pass(context, framebuffer);

	return psl::view_ptr<pass>{element.pass};
}

void render_graph::rebuild() noexcept 
{
	m_Rebuild = false;

}

void render_graph::present()
{
	if(m_Rebuild)
	{
	}

	// todo: we have a failed assumption here till the implementation is done;
	auto it_final = std::find_if(std::begin(m_Passes), std::end(m_Passes),
								 [](const graph_element& element) { return element.pass->is_swapchain(); });

	if(std::distance(std::begin(m_Passes), it_final) != m_Passes.size() - 1)
		std::iter_swap(it_final, std::prev(std::end(m_Passes)));

	
	for(auto& node : m_Passes)
	{
		node.pass->prepare();
		node.pass->build();
		node.pass->present();
	}
}

bool render_graph::connect(psl::view_ptr<pass> child, psl::view_ptr<pass> root) noexcept
{
	auto it_root  = std::find_if(std::begin(m_Passes), std::end(m_Passes),
								 [&root](const graph_element& element) { return &element.pass.get() == root; });
	auto it_child = std::find_if(std::begin(m_Passes), std::end(m_Passes),
								 [&child](const graph_element& element) { return &element.pass.get() == child; });

	if(it_root != std::end(m_Passes) && it_child != std::end(m_Passes))
	{
		it_root->connected_by.emplace_back(child);
		it_root->pass->connect(it_child->pass);
		it_child->connects_to.emplace_back(it_root->pass);
		m_Rebuild = true;
		return true;
	}
	return false;
}

bool render_graph::disconnect(psl::view_ptr<pass> child, psl::view_ptr<pass> root) noexcept
{
	auto it_root  = std::find_if(std::begin(m_Passes), std::end(m_Passes),
								 [&root](const graph_element& element) { return &element.pass.get() == root; });
	auto it_child = std::find_if(std::begin(m_Passes), std::end(m_Passes),
								 [&child](const graph_element& element) { return &element.pass.get() == child; });

	if(it_root != std::end(m_Passes) && it_child != std::end(m_Passes))
	{
		it_root->connected_by.erase(
			std::find(std::begin(it_root->connected_by), std::end(it_root->connected_by), child),
			std::end(it_root->connected_by));
		it_root->pass->disconnect(it_child->pass);
		it_child->connects_to.erase(std::find(std::begin(it_root->connects_to), std::end(it_root->connects_to), root),
									std::end(it_root->connects_to));
		m_Rebuild = true;
		return true;
	}
	return false;
}

bool render_graph::disconnect(psl::view_ptr<pass> child) noexcept
{
	if(auto it_child = std::find_if(std::begin(m_Passes), std::end(m_Passes),
									[&child](const graph_element& element) { return &element.pass.get() == child; });
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

bool render_graph::erase(psl::view_ptr<pass> pass) noexcept
{
	if(auto it = std::find_if(std::begin(m_Passes), std::end(m_Passes),
							  [&pass](const graph_element& element) { return &element.pass.get() == pass; });
	   it != std::end(m_Passes))
	{
		disconnect(pass);
		m_Passes.erase(it);
		m_Rebuild = true;
		return true;
	}
	return false;
}