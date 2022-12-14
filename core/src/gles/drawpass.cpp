#include "gles/drawpass.hpp"
#include "data/material.hpp"
#include "gfx/buffer.hpp"
#include "gfx/drawcall.hpp"
#include "gfx/drawgroup.hpp"
#include "gfx/drawlayer.hpp"
#include "gfx/geometry.hpp"
#include "gfx/material.hpp"
#include "gles/buffer.hpp"
#include "gles/compute.hpp"
#include "gles/computepass.hpp"
#include "gles/conversion.hpp"
#include "gles/framebuffer.hpp"
#include "gles/geometry.hpp"
#include "gles/igles.hpp"
#include "gles/material.hpp"
#include "gles/swapchain.hpp"

#include "data/framebuffer.hpp"

using namespace core::igles;
using namespace core::resource;

drawpass::drawpass(handle<swapchain> swapchain) : m_Swapchain(swapchain) {}
drawpass::drawpass(handle<framebuffer_t> framebuffer) : m_Framebuffer(framebuffer) {}


void drawpass::clear() {
	m_DrawGroups.clear();
}
void drawpass::prepare() {
	PROFILE_SCOPE(core::profiler);
	if(m_Framebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer->framebuffers()[0]);

		unsigned int index {0};
		for(const auto& attachment : m_Framebuffer->data()->attachments()) {
			auto gfx_attachment = attachment.operator core::gfx::attachment();

			auto format		 = gfx_attachment.format;
			auto clear_value = attachment.clear_value();

			bool stencil	   = core::gfx::has_stencil(format);
			bool depth		   = core::gfx::has_depth(format);
			bool depth_stencil = depth && stencil;

			if(depth || stencil) {
				stencil = stencil && gfx_attachment.stencil_load == core::gfx::attachment::load_op::clear;
				depth	= depth && gfx_attachment.image_load == core::gfx::attachment::load_op::clear;

				if(depth_stencil) {
					std::visit(utility::templates::overloaded {[](const core::gfx::depth_stencil& dstencil) {
																   glClearBufferfi(GL_DEPTH_STENCIL,
																				   0,
																				   dstencil.depth,
																				   static_cast<int>(dstencil.stencil));
															   },
															   [](const psl::vec4& color) {},
															   [](const psl::ivec4& color) {},
															   [](const psl::tvec<uint32_t, 4>& color) {

															   }},
							   clear_value);
				} else if(depth) {
					std::visit(utility::templates::overloaded {[](const core::gfx::depth_stencil& dstencil) {
																   glClearBufferfv(GL_DEPTH, 0, &dstencil.depth);
															   },
															   [](const psl::vec4& color) {},
															   [](const psl::ivec4& color) {},
															   [](const psl::tvec<uint32_t, 4>& color) {

															   }},
							   clear_value);
				} else if(stencil) {
					std::visit(utility::templates::overloaded {
								 [](const core::gfx::depth_stencil& dstencil) {
									 glClearBufferiv(GL_STENCIL, 0, reinterpret_cast<const int*>(&dstencil.stencil));
								 },
								 [](const psl::vec4& color) {},
								 [](const psl::ivec4& color) {},
								 [](const psl::tvec<uint32_t, 4>& color) {

								 }},
							   clear_value);
				}
			} else {
				if(gfx_attachment.image_load == core::gfx::attachment::load_op::clear) {
					std::visit(
					  utility::templates::overloaded {
						[&](const core::gfx::depth_stencil& dstencil) {},
						[&](const psl::vec4& color) { glClearBufferfv(GL_COLOR, index, &color[0]); },
						[&](const psl::ivec4& color) { glClearBufferiv(GL_COLOR, index, &color[0]); },
						[&](const psl::tvec<uint32_t, 4>& color) { glClearBufferuiv(GL_COLOR, index, &color[0]); }},
					  clear_value);
				}
				++index;
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	if(m_Swapchain)
		m_Swapchain->clear();
}
bool drawpass::build() {
	return true;
}
void drawpass::present() {
	PROFILE_SCOPE(core::profiler);
	glGetError();
	if(m_Framebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer->framebuffers()[0]);
	}
	for(auto& barrier : m_MemoryBarriers) {
		glMemoryBarrier(barrier.barrier);
	}

	for(const auto& group : m_DrawGroups) {
		for(auto& drawLayer : group.m_Group) {
			// todo: draw call sorting should be done ahead of time in the "build" pass.
			psl::array<uint32_t> render_indices;
			for(auto& drawCall : drawLayer.second) {
				auto matIndices = drawCall.m_Bundle->materialIndices(drawLayer.first.begin(), drawLayer.first.end());
				render_indices.insert(std::end(render_indices), std::begin(matIndices), std::end(matIndices));
			}
			std::sort(std::begin(render_indices), std::end(render_indices));
			render_indices.erase(std::unique(std::begin(render_indices), std::end(render_indices)),
								 std::end(render_indices));
			for(auto renderLayer : render_indices) {
				for(auto& drawCall : drawLayer.second) {
					if(drawCall.m_Geometry.size() == 0 || !drawCall.m_Bundle->has(renderLayer))
						continue;
					auto bundle = drawCall.m_Bundle;

					if(!bundle->bind_material(renderLayer))
						continue;
					auto gfxmat {bundle->bound()};
					auto mat {gfxmat->resource<gfx::graphics_backend::gles>()};

					mat->bind();
					for(auto& [gfxGeometryHandle, count] : drawCall.m_Geometry) {
						auto geometryHandle = gfxGeometryHandle->resource<gfx::graphics_backend::gles>();
						uint32_t instance_n = bundle->instances(gfxGeometryHandle);
						if(instance_n == 0 || !geometryHandle->compatible(mat.value()))
							continue;
						core::profiler.scope_begin("create_vao");
						geometryHandle->create_vao(
						  mat,
						  bundle->m_InstanceData.vertex_buffer()->resource<gfx::graphics_backend::gles>(),
						  bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle));
						core::profiler.scope_end();
						// for(const auto& b : bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle))
						//{
						//	auto buffer =
						// bundle->m_InstanceData.buffer()->resource().get<core::igles::buffer_t>()->id();
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
	auto blend_state = core::data::material_t::blendstate::opaque(0);
	glBlendEquationSeparate(to_gles(blend_state.color_blend_op()), to_gles(blend_state.alpha_blend_op()));
	glBlendFuncSeparate(to_gles(blend_state.color_blend_src()),
						to_gles(blend_state.color_blend_dst()),
						to_gles(blend_state.alpha_blend_src()),
						to_gles(blend_state.alpha_blend_dst()));

	if(m_Framebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	if(m_Swapchain) {
		m_Swapchain->present();
		glFinish();
	}
	glGetError();
}

void drawpass::add(core::gfx::drawgroup& group) noexcept {
	m_DrawGroups.push_back(group);
}

void drawpass::connect(psl::view_ptr<drawpass> pass) noexcept {};
void drawpass::disconnect(psl::view_ptr<drawpass> pass) noexcept {};
void drawpass::connect(psl::view_ptr<computepass> pass) noexcept {
	for(const auto& barrier : pass->memory_barriers()) {
		if(auto it = std::find_if(std::begin(m_MemoryBarriers),
								  std::end(m_MemoryBarriers),
								  [&barrier](auto&& item) { return barrier == item.barrier; });
		   it != std::end(m_MemoryBarriers))
			it->usage += 1;
		else
			m_MemoryBarriers.emplace_back(drawpass::memory_barrier_t {barrier, 1});
	}
};
void drawpass::disconnect(psl::view_ptr<computepass> pass) noexcept {
	for(const auto& barrier : pass->memory_barriers()) {
		if(auto it = std::find_if(std::begin(m_MemoryBarriers),
								  std::end(m_MemoryBarriers),
								  [&barrier](auto&& item) { return barrier == item.barrier; });
		   it != std::end(m_MemoryBarriers)) {
			it->usage -= 1;
			if(it->usage == 0)
				m_MemoryBarriers.erase(it);
		}
	}
};
