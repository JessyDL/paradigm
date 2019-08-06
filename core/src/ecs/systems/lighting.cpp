#include "stdafx.h"
#include "ecs/systems/lighting.h"
#include "ecs/state.h"
#include "memory/region.h"
#include "gfx/pass.h"
#include "vk/context.h"
#include "os/surface.h"
#include "gfx/render_graph.h"
#include "ecs/systems/render.h"
#include "data/buffer.h"
#include "vk/buffer.h"
#include "data/framebuffer.h"
#include "vk/framebuffer.h"
#include "data/sampler.h"
#include "vk/sampler.h"


using namespace core::ecs::systems;
using namespace core;
using namespace core::resource;
using namespace core::ecs::components;
using namespace psl::ecs;

lighting_system::lighting_system(psl::view_ptr<psl::ecs::state> state, psl::view_ptr<core::resource::cache> cache,
								 memory::region& resource_region, psl::view_ptr<core::gfx::render_graph> renderGraph,
								 psl::view_ptr<core::gfx::pass> pass,
								 core::resource::handle<core::ivk::context> context,
								 core::resource::handle<core::os::surface> surface) noexcept
	: m_Cache(cache), m_RenderGraph(renderGraph), m_DependsPass(pass), m_State(state), m_Context(context),
	  m_Surface(surface)
{
	state->declare(&lighting_system::create_dir, this);

	auto bufferData = create<data::buffer>(*cache);
	bufferData.load(vk::BufferUsageFlagBits::eUniformBuffer,
					vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
					resource_region
						.create_region(sizeof(light) * 1024,
									   m_Context->properties().limits.minUniformBufferOffsetAlignment,
									   new memory::default_allocator(true))
						.value());

	m_LightDataBuffer = create<ivk::buffer>(*cache);
	m_LightDataBuffer.load(m_Context, bufferData);
	cache->library().set(m_LightDataBuffer.ID(), "GLOBAL_LIGHT_DATA");

	m_LightSegment = m_LightDataBuffer->reserve(m_LightDataBuffer->free_size()).value();
}

void lighting_system::create_dir(info& info, pack<entity, light, on_combine<light, transform>> pack)
{
	if(pack.size() == 0) return;
	//insertion_sort(std::begin(pack), std::end(pack), sort_impl<light_sort, light>{});

	// create depth pass
	for(auto [e, light] : pack)
	{
		if(!light.shadows) continue;
		auto depthPass = create<gfx::framebuffer>(*m_Cache);
		{
			auto data = create<data::framebuffer>(*m_Cache);
			data.load(m_Surface->data().width(), m_Surface->data().height(), 1);

			{
				vk::AttachmentDescription descr;
				if(utility::vulkan::supported_depthformat(m_Context->physical_device(), &descr.format) != VK_TRUE)
				{
					LOG_FATAL("Could not find a suitable depth stencil buffer format.");
				}
				descr.samples		 = vk::SampleCountFlagBits::e1;
				descr.loadOp		 = vk::AttachmentLoadOp::eClear;
				descr.storeOp		 = vk::AttachmentStoreOp::eDontCare;
				descr.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
				descr.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
				descr.initialLayout  = vk::ImageLayout::eUndefined;
				descr.finalLayout	= vk::ImageLayout::eDepthStencilAttachmentOptimal;

				data->add(m_Surface->data().width(), m_Surface->data().height(), 1,
						  vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ClearDepthStencilValue(1.0f, 0), descr);
			}

			{
				auto ppsamplerData = create<data::sampler>(*m_Cache);
				ppsamplerData.load();
				ppsamplerData->mipmaps(false);
				auto ppsamplerHandle = create<gfx::sampler>(*m_Cache);
				ppsamplerHandle.load(m_Context, ppsamplerData);
				data->set(ppsamplerHandle);
			}

			depthPass.load(m_Context, data);
		}

		m_Passes[e]  = m_RenderGraph->create_pass(m_Context, depthPass);
		m_Systems[e] = new core::ecs::systems::render{*m_State, m_Passes[e]};
		m_Systems[e]->add_render_range(1000, 1500);

		m_RenderGraph->connect(m_Passes[e], m_DependsPass);
	}
};