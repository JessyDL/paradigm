#include "gles/geometry.h"
#include "resource/resource.hpp"
#include "gles/buffer.h"
#include "gles/material.h"
#include "data/geometry.h"
#include "meta/shader.h"
#include "gles/shader.h"
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

void geometry::create_vao(core::resource::handle<core::igles::material> material,
						  core::resource::handle<core::igles::buffer> instanceBuffer,
						  psl::array<std::pair<size_t, size_t>> bindings)
{
	if(auto it = m_VAOs.find(material.ID()); it != std::end(m_VAOs))
	{
		return;
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	auto instance_buffer = instanceBuffer->id();
	glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);

	for(auto binding : bindings)
	{
		for(int index = 0; index < 4; ++index)
		{

			glEnableVertexAttribArray(binding.first + index);
			auto offset = binding.second + (index * sizeof(float) * 4);
			glVertexAttribPointer(binding.first + index, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4,
								  (void*)(offset));
			glVertexAttribDivisor(binding.first + index, 1);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_GeometryBuffer->id());
	std::vector<int> activeSlots{};
	for(const auto& shader : material->shaders())
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
						auto offset = uint64_t{b.segment.range().begin + b.sub_range.begin};

						// todo we need type information here
						glVertexAttribPointer(vBinding.binding_slot(), vBinding.size() / sizeof(GL_FLOAT), GL_FLOAT,
											  false, 0, (void*)offset);
						glEnableVertexAttribArray(vBinding.binding_slot());
						activeSlots.emplace_back(vBinding.binding_slot());
					}
				}
			}
		}
	}
	m_VAOs[material.ID()] = vao;
}

void geometry::bind(core::resource::handle<core::igles::material> material, uint32_t instanceCount)
{
	auto error = glGetError();

	glBindVertexArray(m_VAOs[material.ID()]);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndicesBuffer->id());
	auto indices = m_IndicesSubRange.size() / sizeof(core::data::geometry::index_size_t);
	if(instanceCount == 0)
		glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT,
					   (void*)(m_IndicesSubRange.begin + m_IndicesSegment.range().begin));
	else
		glDrawElementsInstanced(GL_TRIANGLES, indices, GL_UNSIGNED_INT,
								(void*)(m_IndicesSubRange.begin + m_IndicesSegment.range().begin), instanceCount);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
};