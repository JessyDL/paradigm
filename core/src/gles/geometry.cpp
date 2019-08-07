#include "gles/geometry.h"
#include "resource/resource.hpp"
#include "gles/buffer.h"
#include "data/geometry.h"
#include "array.h"
#include "glad/glad_wgl.h"

using namespace core::igles;
using namespace core::resource;

using gData = core::data::geometry;

geometry::geometry(psl::UID uid, cache& cache, handle<gData> data, handle<buffer> vertexBuffer,
				   handle<buffer> indexBuffer)
	: m_GeometryBuffer(vertexBuffer), m_IndicesBuffer(indexBuffer)
{
	psl::array<size_t> sizeRequests;
	sizeRequests.reserve(data->vertex_streams().size() + ((vertexBuffer == indexBuffer) ? 1 : 0));
	std::for_each(std::begin(data->vertex_streams()), std::end(data->vertex_streams()),
				  [&sizeRequests](const std::pair<psl::string, core::stream>& element) {
					  sizeRequests.emplace_back(element.second.bytesize());
				  });
	auto error = glGetError();

	if(vertexBuffer == indexBuffer)
	{
		sizeRequests.emplace_back((uint32_t)(data->indices().size() * sizeof(core::data::geometry::index_size_t)));
	}

	auto segments = vertexBuffer->allocate(sizeRequests, true);

	std::vector<core::gfx::memory_copy> instructions;
	auto current_segment = std::begin(segments);
	for(const auto& stream : data->vertex_streams())
	{
		auto& instr				 = instructions.emplace_back();
		instr.size				 = stream.second.bytesize();
		instr.destination_offset = current_segment->first.range().begin + current_segment->second.begin;
		instr.source_offset		 = (std::uintptr_t)(stream.second.cdata());

		auto& b			= m_Bindings.emplace_back();
		b.name			= stream.first;
		b.segment		= current_segment->first;
		b.sub_range		= current_segment->second;
		current_segment = std::next(current_segment);
	}

	if(vertexBuffer != indexBuffer)
	{
		if(auto indiceSegment =
			   indexBuffer->allocate(data->indices().size() * sizeof(core::data::geometry::index_size_t));
		   indiceSegment)
		{
			m_IndicesSegment  = indiceSegment.value();
			m_IndicesSubRange = memory::range{0, m_IndicesSegment.range().size()};
		}
		else
		{
			// todo error condition could not allocate segment
			exit(1);
		}

		indexBuffer->set({{(size_t)data->indices().data(), m_IndicesSegment.range().begin,
						   data->indices().size() * sizeof(core::data::geometry::index_size_t)}});
	}
	else
	{
		auto& instr				 = instructions.emplace_back();
		instr.size				 = data->indices().size() * sizeof(decltype(data->indices().at(0)));
		instr.destination_offset = current_segment->first.range().begin + current_segment->second.begin;
		instr.source_offset		 = (std::uintptr_t)(data->indices().data());

		m_IndicesSegment  = current_segment->first;
		m_IndicesSubRange = current_segment->second;
	}

	vertexBuffer->set(instructions);
	error = glGetError();
}

geometry::~geometry()
{
	if(!m_GeometryBuffer) return;

	for(auto& binding : m_Bindings)
	{
		// this check makes sure this is the owner of the memory segment. if there's a local offset it means this
		// binding has a shared memory::segment and we should only clear the first one who coincidentally starts at
		// begin 0
		if(binding.sub_range.begin == 0) m_GeometryBuffer->deallocate(binding.segment);
	}
	// same as the earlier comment for geometry buffer
	if(m_IndicesSubRange.begin == 0) m_IndicesBuffer->deallocate(m_IndicesSegment);
}

void geometry::bind()
{
	auto error = glGetError();
	glBindBuffer(GL_ARRAY_BUFFER, m_GeometryBuffer->id());

	for(const auto& binding : m_Bindings)
	{
		if(binding.name == core::data::geometry::constants::POSITION)
		{
			// this is a temporary hack
			glVertexAttribPointer(
				0, 3, GL_FLOAT, false, 0,
				(void*)(binding.segment.range().begin + binding.sub_range.begin)); // <----- 0, because "vbo" is bound

			error = glGetError();
			glEnableVertexAttribArray(0);
		}
		else if(binding.name == core::data::geometry::constants::COLOR)
		{
			glVertexAttribPointer(
				1, 3, GL_FLOAT, false, 0,
				(void*)(binding.segment.range().begin + binding.sub_range.begin)); // <----- 0, because "vbo" is bound
			glEnableVertexAttribArray(1);
		
		}
		else if(binding.name == core::data::geometry::constants::TEX)
		{
			glVertexAttribPointer(
				2, 2, GL_FLOAT, true, 0,
				(void*)(binding.segment.range().begin + binding.sub_range.begin)); // <----- 0, because "vbo" is bound
			glEnableVertexAttribArray(2);
		}
	}


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndicesBuffer->id());
	auto indices = m_IndicesSubRange.size() / sizeof(core::data::geometry::index_size_t);
	glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT,
				   (void*)(m_IndicesSubRange.begin + m_IndicesSegment.range().begin));

	for(const auto& binding : m_Bindings)
	{
		if(binding.name == core::data::geometry::constants::POSITION)
		{
			glDisableVertexAttribArray(0);
		}
		else if(binding.name == core::data::geometry::constants::COLOR)
		{
			glDisableVertexAttribArray(1);
		}
		else if(binding.name == core::data::geometry::constants::TEX)
		{
			glDisableVertexAttribArray(2);
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
};