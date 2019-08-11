#include "gles/pass.h"
#include "gles/swapchain.h"
#include "gles/material.h"
#include "gles/geometry.h"
#include "gles/buffer.h"
#include "gfx/drawgroup.h"
#include "gfx/drawlayer.h"
#include "gfx/drawcall.h"
#include "gfx/material.h"
#include "gfx/geometry.h"
#include "gfx/buffer.h"
#include "glad/glad_wgl.h"

using namespace core::igles;
using namespace core::resource;

pass::pass(handle<swapchain> swapchain) : m_Swapchain(swapchain) {}


void pass::clear()
{
	m_Swapchain->clear();
}
void pass::prepare() {}
bool pass::build() { return true; }
void pass::present()
{
	glFinish();
	for(const auto& group : m_DrawGroups)
	{
		for(auto& drawLayer : group.m_Group)
		{
			for(auto& drawCall : drawLayer.second)
			{
				if(drawCall.m_Geometry.size() == 0) continue;
				auto bundle = drawCall.m_Bundle;

				auto matIndices = drawCall.m_Bundle->materialIndices(drawLayer.first.begin(), drawLayer.first.end());

				for(auto index : matIndices)
				{
					bundle->bind_material(index);
					auto gfxmat{bundle->bound()};
					auto mat{gfxmat->resource().get<core::igles::material>()};

					mat->bind();
					for(auto& [gfxGeometryHandle, count] : drawCall.m_Geometry)
					{
						auto geometryHandle = gfxGeometryHandle->resource().get<core::igles::geometry>();
						uint32_t instance_n = bundle->instances(gfxGeometryHandle);
						if(instance_n == 0 /*|| !geometryHandle->compatible(mat)*/) continue;

						for(const auto& b : bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle))
						{
							auto buffer = bundle->m_InstanceData.buffer()->resource().get<core::igles::buffer>()->id();
							glBindBuffer(GL_ARRAY_BUFFER, buffer);
							for(int i = 0; i < 4; ++i)
							{

								glEnableVertexAttribArray(b.first + i);
								glVertexAttribPointer(b.first +i , 4, GL_FLOAT, GL_FALSE, 16 * sizeof(GL_FLOAT),
													  (void*)(b.second + (i * sizeof(GL_FLOAT) * 4)));
								glVertexAttribDivisor(b.first +i , 1);
							}
							glBindBuffer(GL_ARRAY_BUFFER, 0);
						}

						geometryHandle->bind(mat, instance_n);

						for(const auto& b : bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle))
						{
							glDisableVertexAttribArray(b.first);
						}
					}
				}
			}
		}
	}
	glFinish();
	m_Swapchain->present();
}

void pass::add(core::gfx::drawgroup& group) noexcept { m_DrawGroups.push_back(group); }