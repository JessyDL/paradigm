#include "logging.h"
#include "vk/geometry.h"
#include "data/geometry.h"
#include "vk/buffer.h"
#include "data/buffer.h"
#include "vk/material.h"
#include "data/material.h"
#include "vk/shader.h"
#include "meta/shader.h"
#include "resource/resource.hpp"

using namespace psl;
using namespace core::resource;
using namespace core::gfx;
using namespace core::ivk;

geometry::geometry(const UID& uid, cache& cache, handle<core::ivk::context> context,
				   core::resource::handle<core::data::geometry> data,
				   core::resource::handle<core::ivk::buffer> geometryBuffer,
				   core::resource::handle<core::ivk::buffer> indicesBuffer)
	: m_Context(context), m_Data(data), m_GeometryBuffer(geometryBuffer), m_IndicesBuffer(indicesBuffer), m_UID(uid)
{
	std::vector<vk::DeviceSize> sizeRequests;
	sizeRequests.reserve(m_Data->vertex_streams().size() + ((m_GeometryBuffer == m_IndicesBuffer)?1:0));
	std::for_each(std::begin(m_Data->vertex_streams()), std::end(m_Data->vertex_streams()), [&sizeRequests](const std::pair<psl::string, core::stream>& element)
				  {
					  sizeRequests.emplace_back((uint32_t)element.second.bytesize());
				  });

	if(m_GeometryBuffer == m_IndicesBuffer)
	{
		sizeRequests.emplace_back((uint32_t)(m_Data->indices().size() * sizeof(core::data::geometry::index_size_t)));
	}

	auto segments = m_GeometryBuffer->reserve(sizeRequests, true);
	std::vector<buffer::commit_instruction> instructions;
	size_t i = 0;
	for(const auto& stream : m_Data->vertex_streams())
	{
		auto& instr = instructions.emplace_back();
		instr.size = stream.second.bytesize();
		instr.source = (std::uintptr_t)(stream.second.cdata());
		instr.segment = segments[i].first;
		if(segments[i].first.range() != segments[i].second)
			instr.sub_range = segments[i].second;

		auto& b = m_Bindings.emplace_back();
		b.name = stream.first;
		b.segment = segments[i].first;
		b.sub_range = segments[i].second;
		++i;
	}

	if(m_GeometryBuffer != m_IndicesBuffer)
	{
		if(auto indiceSegment = m_IndicesBuffer->reserve((uint32_t)(m_Data->indices().size() * sizeof(core::data::geometry::index_size_t))); indiceSegment)
		{
			m_IndicesSegment = indiceSegment.value();
			m_IndicesSubRange = memory::range{0, m_IndicesSegment.range().size()};
		}
		else
		{
			// todo error condition could not allocate segment
			exit(1);
		}
		buffer::commit_instruction instr;
		instr.size = m_IndicesSegment.range().size();
		instr.source = (std::uintptr_t)m_Data->indices().data();
		instr.segment = m_IndicesSegment;
		m_IndicesBuffer->commit({instr});
	}
	else
	{
		i = segments.size() - 1;
		auto& instr = instructions.emplace_back();
		instr.size = segments[i].second.size();
		instr.source = (std::uintptr_t)m_Data->indices().data();
		instr.segment = segments[i].first;
		if(segments[i].first.range() != segments[i].second)
			instr.sub_range = segments[i].second;

		m_IndicesSegment = segments[i].first;
		m_IndicesSubRange = segments[i].second;
	}

	m_GeometryBuffer->commit(instructions);
}

geometry::~geometry() 
{
	if(!m_GeometryBuffer)
		return;

	for(auto& binding : m_Bindings)
	{
		// this check makes sure this is the owner of the memory segment. if there's a local offset it means this binding has a shared 
		// memory::segment and we should only clear the first one who coincidentally starts at begin 0
		if(binding.sub_range.begin == 0)
			m_GeometryBuffer->deallocate(binding.segment);
	}
	// same as the earlier comment for geometry buffer
	if(m_IndicesSubRange.begin == 0)
		m_IndicesBuffer->deallocate(m_IndicesSegment);
}


bool geometry::compatible(const core::ivk::material& material) const noexcept
{
	for(const auto& shader : material.shaders())
	{
		if(shader->meta()->stage() == core::gfx::shader_stage::vertex)
		{
			for(const auto& vBinding : shader->meta()->vertex_bindings())
			{
				if(vBinding.input_rate() != core::gfx::vertex_input_rate::vertex) continue;

				for(const auto& b : m_Bindings)
				{
					if(psl::to_string8_t(b.name) == vBinding.buffer())
					{
						goto success;
					}
				}
				goto error;

				success:
				continue;

			error:
				core::gfx::log->error( "missing ATTRIBUTE [{0}] in GEOMETRY [{1}]" , vBinding.buffer(), utility::to_string(m_UID));
				return false;
			}
		}
	}
	return true;
}

void geometry::bind(vk::CommandBuffer& buffer, const core::ivk::material& material) const noexcept
{

	for(const auto& shader: material.shaders())
	{
		const auto& meta = shader->meta();
		if(meta->stage() == core::gfx::shader_stage::vertex) // TODO: check if possible on fragment shader etc..
		{
			for(auto& vBinding : meta->vertex_bindings())
			{
				if(vBinding.input_rate() != core::gfx::vertex_input_rate::vertex) continue;

				for(const auto& b : m_Bindings)
				{
					if(psl::to_string8_t(b.name) == vBinding.buffer())
					{
						auto offset = vk::DeviceSize{b.segment.range().begin + b.sub_range.begin};
						buffer.bindVertexBuffers(vBinding.binding_slot(), 1, &m_GeometryBuffer->gpu_buffer(), &offset);
					}
				}
			}
		}
	}

	buffer.bindIndexBuffer(m_IndicesBuffer->gpu_buffer(), m_IndicesSegment.range().begin + m_IndicesSubRange.begin, vk::IndexType::eUint32);
}