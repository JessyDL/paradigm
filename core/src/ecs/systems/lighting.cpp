#include "stdafx.h"
#include "ecs/systems/lighting.h"
#include "psl/ecs/state.h"
#include "psl/memory/region.h"
#include "gfx/pass.h"
#include "gfx/context.h"
#include "os/surface.h"
#include "gfx/render_graph.h"
#include "ecs/systems/render.h"
#include "data/buffer.h"
#include "gfx/buffer.h"
#include "data/framebuffer.h"
#include "gfx/framebuffer.h"
#include "data/sampler.h"
#include "gfx/sampler.h"
#include "gfx/limits.h"

using namespace core::ecs::systems;
using namespace core;
using namespace core::resource;
using namespace core::ecs::components;
using namespace psl::ecs;

lighting_system::lighting_system(psl::view_ptr<psl::ecs::state> state, psl::view_ptr<core::resource::cache> cache,
								 memory::region& resource_region, psl::view_ptr<core::gfx::render_graph> renderGraph,
								 psl::view_ptr<core::gfx::pass> pass,
								 core::resource::handle<core::gfx::context> context,
								 core::resource::handle<core::os::surface> surface) noexcept
	: m_Cache(cache), m_RenderGraph(renderGraph), m_DependsPass(pass), m_State(state), m_Context(context),
	  m_Surface(surface)
{
	state->declare(&lighting_system::create_dir, this);

	auto bufferData = cache->create<data::buffer>(
		gfx::memory_usage::uniform_buffer, gfx::memory_property::host_visible | gfx::memory_property::host_coherent,
		resource_region
			.create_region(sizeof(light) * 1024, core::gfx::limits::uniform_buffer_offset_alignment(m_Context.value()),
						   new memory::default_allocator(true))
			.value());

	m_LightDataBuffer = cache->create<gfx::buffer>(m_Context, bufferData);
	cache->library().set(m_LightDataBuffer, "GLOBAL_LIGHT_DATA");
	m_LightSegment = m_LightDataBuffer->reserve(m_LightDataBuffer->free_size()).value();
}

void lighting_system::create_dir(info& info, pack<entity, light, on_combine<light, transform>> pack)
{
	if(pack.size() == 0) return;
	// insertion_sort(std::begin(pack), std::end(pack), sort_impl<light_sort, light>{});

	// create depth pass
	for(auto [e, light] : pack)
	{
		if(!light.shadows) continue;

		auto fbdata = m_Cache->create<data::framebuffer>(m_Surface->data().width(), m_Surface->data().height(), 1);

		{
			vk::AttachmentDescription descr;
			if(auto format = core::gfx::limits::supported_depthformat(m_Context.value());
			   format == core::gfx::format::undefined)
			{
				LOG_FATAL("Could not find a suitable depth stencil buffer format.");
			}
			else
				descr.format = core::gfx::conversion::to_vk(format);
			descr.samples		 = vk::SampleCountFlagBits::e1;
			descr.loadOp		 = vk::AttachmentLoadOp::eClear;
			descr.storeOp		 = vk::AttachmentStoreOp::eDontCare;
			descr.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
			descr.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			descr.initialLayout  = vk::ImageLayout::eUndefined;
			descr.finalLayout	= vk::ImageLayout::eDepthStencilAttachmentOptimal;

			fbdata->add(m_Surface->data().width(), m_Surface->data().height(), 1,
						vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ClearDepthStencilValue(1.0f, 0), descr);
		}

		{
			auto ppsamplerData = m_Cache->create<data::sampler>();
			ppsamplerData->mipmaps(false);
			auto ppsamplerHandle = m_Cache->create<gfx::sampler>(m_Context, ppsamplerData);
			fbdata->set(ppsamplerHandle);
		}

		auto depthPass = m_Cache->create<gfx::framebuffer>(m_Context, fbdata);

		m_Passes[e]  = m_RenderGraph->create_pass(m_Context, depthPass);
		m_Systems[e] = new core::ecs::systems::render{*m_State, m_Passes[e]};
		m_Systems[e]->add_render_range(1000, 1500);

		m_RenderGraph->connect(m_Passes[e], m_DependsPass);
	}
};