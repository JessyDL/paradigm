#include "gfx/render_graph.h"
#include "gfx/pass.h"
#include "vk/context.h"
#include "vk/swapchain.h"
#include "vk/framebuffer.h"

using namespace core::gfx;
using core::resource::handle;


pass& render_graph::create_pass(handle<context> context, handle<swapchain> swapchain)
{
	auto& element = m_Passes.emplace_back();
	element.pass = new pass(context, swapchain);

	return *element.pass;
}
pass& render_graph::create_pass(handle<context> context, handle<framebuffer> framebuffer)
{
	auto& element = m_Passes.emplace_back();
	element.pass = new pass(context, framebuffer);

	return *element.pass;
}

void render_graph::present()
{
	// rebuild the graph
	if (m_Graph.size() == 0 && m_Passes.size() > 0)
	{

	}

	// todo: we have a failed assumption here till the implementation is done;
	auto it_root = std::find_if(std::begin(m_Passes), std::end(m_Passes), [](const graph_element& element) { return element.dependencies.size() == 0; });
	auto it_final = std::find_if(std::begin(m_Passes), std::end(m_Passes), [](const graph_element& element) { return element.dependencies.size() > 0; });

	it_root->pass->prepare();
	it_root->pass->build();
	it_root->pass->present();

	it_final->pass->prepare();
	it_final->pass->build();
	it_final->pass->present();
}

bool render_graph::add_dependency(core::gfx::pass& root, core::gfx::pass& child) noexcept
{
	auto it_root = std::find_if(std::begin(m_Passes), std::end(m_Passes), [&root](const graph_element& element) { return &element.pass.get() == &root; });
	auto it_child = std::find_if(std::begin(m_Passes), std::end(m_Passes),  [&child](const graph_element& element) { return &element.pass.get() == &child; });

	if (it_root != std::end(m_Passes) && it_child != std::end(m_Passes))
	{
		it_child->dependencies.emplace_back(&child);
		it_child->pass->depends_on(*(it_root->pass));
		return true;
	}
	return false;
}