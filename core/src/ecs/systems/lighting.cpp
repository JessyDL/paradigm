#include "ecs/systems/lighting.hpp"
#include "data/buffer.hpp"
#include "data/framebuffer.hpp"
#include "data/sampler.hpp"
#include "ecs/systems/render.hpp"
#include "gfx/buffer.hpp"
#include "gfx/context.hpp"
#include "gfx/drawpass.hpp"
#include "gfx/framebuffer.hpp"
#include "gfx/limits.hpp"
#include "gfx/render_graph.hpp"
#include "gfx/sampler.hpp"
#include "os/surface.hpp"
#include "psl/ecs/state.hpp"
#include "psl/memory/region.hpp"

using namespace core::ecs::systems;
using namespace core;
using namespace core::resource;
using namespace core::ecs::components;
using namespace psl::ecs;

lighting_system::lighting_system(psl::view_ptr<psl::ecs::state_t> state,
								 psl::view_ptr<core::resource::cache_t> cache,
								 memory::region& resource_region,
								 psl::view_ptr<core::gfx::render_graph> renderGraph,
								 psl::view_ptr<core::gfx::drawpass> pass,
								 core::resource::handle<core::gfx::context> context,
								 core::resource::handle<core::os::surface> surface) noexcept
	: m_Cache(cache), m_RenderGraph(renderGraph), m_DependsPass(pass), m_State(state), m_Context(context),
	  m_Surface(surface) {
	state->declare(&lighting_system::create_dir, this);

	auto bufferData = cache->create<data::buffer_t>(
	  gfx::memory_usage::uniform_buffer,
	  gfx::memory_property::host_visible | gfx::memory_property::host_coherent,
	  resource_region
		.create_region(sizeof(light) * 1024, m_Context->limits().uniform.alignment, new memory::default_allocator(true))
		.value());

	m_LightDataBuffer = cache->create<gfx::buffer_t>(m_Context, bufferData);
	cache->library().set(m_LightDataBuffer, "GLOBAL_LIGHT_DATA");
	m_LightSegment = m_LightDataBuffer->reserve(m_LightDataBuffer->free_size()).value();
}

void lighting_system::create_dir(info_t& info, pack_direct_full_t<entity_t, light, on_combine<light, transform>> pack) {
	if(pack.size() == 0)
		return;
	// insertion_sort(std::begin(pack), std::end(pack), sort_impl<light_sort, light>{});

	// create depth pass
	for(auto [e, light] : pack) {
		if(!light.shadows)
			continue;

		auto fbdata = m_Cache->create<data::framebuffer_t>(m_Surface->data().width(), m_Surface->data().height(), 1);

		{
			core::gfx::attachment descr;
			if(auto format = m_Context->limits().supported_depthformat; format == core::gfx::format_t::undefined) {
				core::log->error("Could not find a suitable depth stencil buffer format.");
			} else
				descr.format = format;
			descr.sample_bits	= 1;
			descr.image_load	= core::gfx::attachment::load_op::clear;
			descr.image_store	= core::gfx::attachment::store_op::dont_care;
			descr.stencil_load	= core::gfx::attachment::load_op::dont_care;
			descr.stencil_store = core::gfx::attachment::store_op::dont_care;
			descr.initial		= core::gfx::image::layout::undefined;
			descr.final			= core::gfx::image::layout::depth_stencil_attachment_optimal;

			fbdata->add(m_Surface->data().width(),
						m_Surface->data().height(),
						1,
						core::gfx::image::usage::dept_stencil_attachment,
						core::gfx::depth_stencil {1.0f, 0},
						descr);
		}

		{
			auto ppsamplerData = m_Cache->create<data::sampler_t>();
			ppsamplerData->mipmaps(false);
			auto ppsamplerHandle = m_Cache->create<gfx::sampler_t>(m_Context, ppsamplerData);
			fbdata->set(ppsamplerHandle);
		}

		auto depthPass = m_Cache->create<gfx::framebuffer_t>(m_Context, fbdata);

		auto pass											  = m_RenderGraph->create_drawpass(m_Context, depthPass);
		m_Systems[static_cast<psl::ecs::entity_size_type>(e)] = new core::ecs::systems::render {*m_State, pass};
		m_Systems[static_cast<psl::ecs::entity_size_type>(e)]->add_render_range(1000, 1500);

		m_RenderGraph->connect(pass, m_DependsPass);
	}
};
