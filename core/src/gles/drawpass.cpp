#include "gles/drawpass.h"
#include "gles/swapchain.h"
#include "gles/framebuffer.h"
#include "gles/material.h"
#include "gles/geometry.h"
#include "gles/buffer.h"
#include "gles/compute.h"
#include "gfx/drawgroup.h"
#include "gfx/drawlayer.h"
#include "gfx/drawcall.h"
#include "gfx/material.h"
#include "gfx/geometry.h"
#include "gfx/buffer.h"
#include "glad/glad_wgl.h"
#include "data/material.h"
#include "gles/conversion.h"
#include "gles/computepass.h"

using namespace core::igles;
using namespace core::resource;

drawpass::drawpass(handle<swapchain> swapchain) : m_Swapchain(swapchain) {}
drawpass::drawpass(handle<framebuffer> framebuffer) : m_Framebuffer(framebuffer) {}


void drawpass::clear() { m_DrawGroups.clear(); }
void drawpass::prepare()
{
	if(m_Framebuffer)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer->framebuffers()[0]);
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearDepthf(1.0f);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	if(m_Swapchain) m_Swapchain->clear();
}
bool drawpass::build() { return true; }
void drawpass::present()
{
	glGetError();
	if(m_Framebuffer)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer->framebuffers()[0]);
	}
	for(auto& barrier : m_MemoryBarriers)
	{
		glMemoryBarrier(barrier.barrier);
	}

	for(const auto& group : m_DrawGroups)
	{
		for(auto& drawLayer : group.m_Group)
		{
			// todo: draw call sorting should be done ahead of time in the "build" pass.
			psl::array<uint32_t> render_indices;
			for(auto& drawCall : drawLayer.second)
			{
				auto matIndices = drawCall.m_Bundle->materialIndices(drawLayer.first.begin(), drawLayer.first.end());
				render_indices.insert(std::end(render_indices), std::begin(matIndices), std::end(matIndices));
			}
			std::sort(std::begin(render_indices), std::end(render_indices));
			render_indices.erase(std::unique(std::begin(render_indices), std::end(render_indices)), std::end(render_indices));
			for(auto renderLayer : render_indices)
			{
				for(auto& drawCall : drawLayer.second)
				{
					if(drawCall.m_Geometry.size() == 0 || !drawCall.m_Bundle->has(renderLayer)) continue;
					auto bundle = drawCall.m_Bundle;

					bundle->bind_material(renderLayer);
					auto gfxmat{bundle->bound()};
					auto mat{gfxmat->resource().get<core::igles::material>()};

					mat->bind();
					for(auto& [gfxGeometryHandle, count] : drawCall.m_Geometry)
					{
						auto geometryHandle = gfxGeometryHandle->resource().get<core::igles::geometry>();
						uint32_t instance_n = bundle->instances(gfxGeometryHandle);
						if(instance_n == 0 || !geometryHandle->compatible(mat.value())) continue;

						geometryHandle->create_vao(
							mat, bundle->m_InstanceData.buffer()->resource().get<core::igles::buffer>(),
							bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle));
						// for(const auto& b : bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle))
						//{
						//	auto buffer = bundle->m_InstanceData.buffer()->resource().get<core::igles::buffer>()->id();
						//	glBindBuffer(GL_ARRAY_BUFFER, buffer);
						//	glEnableVertexAttribArray(b.first);
						//	glEnableVertexAttribArray(b.first + 1);
						//	glEnableVertexAttribArray(b.first + 2);
						//	glEnableVertexAttribArray(b.first + 3);
						//	for(int i = 0; i < 4; ++i)
						//	{
						//		glGetError();
						//		auto offset = b.second + (i * sizeof(float) * 4);
						//		//glVertexAttribPointer(b.first + i, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float),
						//		//					  (void*)(offset));

						//		glVertexAttribPointer(b.first, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4,
						//							  (void*)(0));
						//		glGetError();
						//		glVertexAttribDivisor(b.first + i, 1);
						//		glGetError();
						//	}
						//	glBindBuffer(GL_ARRAY_BUFFER, 0);
						//}

						geometryHandle->bind(mat, instance_n);

						/*for(const auto& b : bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle))
						{
							glDisableVertexAttribArray(b.first);
						}*/
					}
				}
			}
		}
	}
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);
	glDepthMask(true);
	using namespace core::gfx::conversion;
	auto blend_state = core::data::material::blendstate::opaque(0);
	glBlendEquationSeparate(to_gles(blend_state.color_blend_op()), to_gles(blend_state.alpha_blend_op()));
	glBlendFuncSeparate(to_gles(blend_state.color_blend_src()), to_gles(blend_state.color_blend_dst()),
						to_gles(blend_state.alpha_blend_src()), to_gles(blend_state.alpha_blend_dst()));
	if(m_Framebuffer)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	if(m_Swapchain) m_Swapchain->present();
	glGetError();
}

void drawpass::add(core::gfx::drawgroup& group) noexcept { m_DrawGroups.push_back(group); }

void drawpass::connect(psl::view_ptr<drawpass> pass) noexcept {};
void drawpass::disconnect(psl::view_ptr<drawpass> pass) noexcept {};
void drawpass::connect(psl::view_ptr<computepass> pass) noexcept
{
	for (const auto& barrier : pass->memory_barriers())
	{
		if (auto it = std::find_if(std::begin(m_MemoryBarriers), std::end(m_MemoryBarriers),
			[&barrier](auto&& item) { return barrier == item.barrier; });
			it != std::end(m_MemoryBarriers))
			it->usage += 1;
		else
			m_MemoryBarriers.emplace_back(drawpass::memory_barrier_t{ barrier, 1 });
	}
};
void drawpass::disconnect(psl::view_ptr<computepass> pass) noexcept 
{
	for (const auto& barrier : pass->memory_barriers())
	{
		if (auto it = std::find_if(std::begin(m_MemoryBarriers), std::end(m_MemoryBarriers),
			[&barrier](auto&& item) { return barrier == item.barrier; });
			it != std::end(m_MemoryBarriers))
		{
			it->usage -= 1;
			if (it->usage == 0)
				m_MemoryBarriers.erase(it);
		}
	}
};