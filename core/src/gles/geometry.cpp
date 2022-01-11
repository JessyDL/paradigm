#include "gles/geometry.h"
#include "data/geometry.h"
#include "data/material.h"
#include "gles/buffer.h"
#include "gles/igles.h"
#include "gles/material.h"
#include "gles/shader.h"
#include "logging.h"
#include "meta/shader.h"
#include "psl/array.h"
#include "resource/resource.hpp"

using namespace core::igles;
using namespace core::resource;

using gData = core::data::geometry_t;

geometry_t::geometry_t(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   handle<gData> data,
				   handle<buffer_t> vertexBuffer,
				   handle<buffer_t> indexBuffer) :
	m_UID(metaData.uid),
	m_GeometryBuffer(vertexBuffer), m_IndicesBuffer(indexBuffer)
{
	recreate(data);
}

geometry_t::~geometry_t()
{
	if(!m_GeometryBuffer) return;

	clear(true);
}

void geometry_t::clear(bool including_vao)
{
	if(including_vao)
	{
		for(auto& [uid, vao] : m_VAOs) glDeleteVertexArrays(1, &vao);

		m_VAOs.clear();
	}
	for(auto& binding : m_Bindings)
	{
		// this check makes sure this is the owner of the memory segment. if there's a local offset it means this
		// binding has a shared memory::segment and we should only clear the first one who coincidentally starts at
		// begin 0
		if(binding.sub_range.begin == 0 && binding.sub_range.size() != 0) m_GeometryBuffer->deallocate(binding.segment);
		binding.segment	  = {};
		binding.sub_range = {};
	}
	// same as the earlier comment for geometry buffer
	if(m_IndicesBuffer && m_IndicesSubRange.begin == 0 && m_IndicesSubRange.size() != 0)
		m_IndicesBuffer->deallocate(m_IndicesSegment);
	m_IndicesSegment = {};
	m_Bindings		 = {};
}
void geometry_t::recreate(core::resource::handle<core::data::geometry_t> data)
{
	clear(false);

	psl::array<size_t> sizeRequests;
	sizeRequests.reserve(data->vertex_streams().size() + ((m_GeometryBuffer == m_IndicesBuffer) ? 1 : 0));
	std::for_each(std::begin(data->vertex_streams()),
				  std::end(data->vertex_streams()),
				  [&sizeRequests](const std::pair<psl::string, core::stream>& element) {
					  sizeRequests.emplace_back(element.second.bytesize());
				  });
	auto error = glGetError();

	if(m_GeometryBuffer == m_IndicesBuffer)
	{
		sizeRequests.emplace_back((uint32_t)(data->indices().size() * sizeof(core::data::geometry_t::index_size_t)));
	}

	auto segments = m_GeometryBuffer->allocate(sizeRequests, true);
	if(segments.size() == 0)
	{
		core::igles::log->critical("ran out of memory, could not allocate enough in the buffer to accomodate");
		exit(1);
	}
	assert(data->vertex_streams().size() > 0);
	m_Vertices = std::begin(data->vertex_streams())->second.size();
	std::vector<core::gfx::memory_copy> instructions;
	auto current_segment = std::begin(segments);
	for(const auto& stream : data->vertex_streams())
	{
		assert(m_Vertices == stream.second.size());
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

	if(m_GeometryBuffer != m_IndicesBuffer)
	{
		if(auto indiceSegment =
			 m_IndicesBuffer->allocate(data->indices().size() * sizeof(core::data::geometry_t::index_size_t));
		   indiceSegment)
		{
			m_IndicesSegment  = indiceSegment.value();
			m_IndicesSubRange = memory::range_t {0, m_IndicesSegment.range().size()};
		}
		else
		{
			core::igles::log->critical("index buffer was out of memory");
			// todo error condition could not allocate segment
			exit(1);
		}

		m_IndicesBuffer->set({{(size_t)data->indices().data(),
							   m_IndicesSegment.range().begin,
							   data->indices().size() * sizeof(core::data::geometry_t::index_size_t)}});
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

	m_Triangles = (m_IndicesSubRange.size() * sizeof(core::data::geometry_t::index_size_t)) / 3u;

	m_GeometryBuffer->set(instructions);
	error = glGetError();
}
void geometry_t::recreate(core::resource::handle<core::data::geometry_t> data,
						core::resource::handle<core::igles::buffer_t> vertexBuffer,
						core::resource::handle<core::igles::buffer_t> indexBuffer)
{
	clear();
	m_GeometryBuffer = vertexBuffer;
	m_IndicesBuffer	 = indexBuffer;

	recreate(data);
}

void geometry_t::create_vao(core::resource::handle<core::igles::material_t> material,
						  core::resource::handle<core::igles::buffer_t> instanceBuffer,
						  psl::array<std::pair<size_t, size_t>> bindings)
{
	GLuint vao;
	// todo this is not correct
	// issue: when a material gets their instance bindings updated, the VAO doesn't know
	if(auto it = m_VAOs.find(material); it != std::end(m_VAOs))
	{
		vao = it->second;
	}
	else
	{
		glGenVertexArrays(1, &vao);
	}
	glBindVertexArray(vao);

	auto instance_buffer = instanceBuffer->id();
	glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);

	for(auto binding : bindings)
	{
		core::log->info("binding id {} offset {}", binding.first, binding.second);
		for(int index = 0; index < 4; ++index)
		{
			glEnableVertexAttribArray(binding.first + index);
			auto offset = binding.second + (index * sizeof(float) * 4);
			glVertexAttribPointer(
			  binding.first + index, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4, (void*)(offset));
			glVertexAttribDivisor(binding.first + index, 1);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_GeometryBuffer->id());

	for(const auto& stage : material->data().stages())
	{
		auto meta_it = std::find_if(std::begin(material->shaders()),
									std::end(material->shaders()),
									[uid = stage.shader()](const auto& shader) { return shader->meta()->ID() == uid; });

		for(const auto& attribute : stage.attributes())
		{
			for(const auto& b : m_Bindings)
			{
				if(psl::to_string8_t(b.name) == attribute.tag())
				{
					auto offset = uint64_t {b.segment.range().begin + b.sub_range.begin};

					auto input = std::find_if(
					  std::begin(meta_it->meta()->inputs()),
					  std::end(meta_it->meta()->inputs()),
					  [location = attribute.location()](const auto& input) { return location == input.location(); });
					// todo we need type information here
					glEnableVertexAttribArray(attribute.location());
					glVertexAttribPointer(
					  attribute.location(), input->size() / sizeof(GL_FLOAT), GL_FLOAT, GL_FALSE, 0, (void*)offset);
				}
			}
		}
	}
	m_VAOs[material] = vao;
}
void geometry_t::bind(core::resource::handle<core::igles::material_t> material, uint32_t instanceCount)
{
	auto error = glGetError();

	glBindVertexArray(m_VAOs[material]);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndicesBuffer->id());
	auto indices = m_IndicesSubRange.size() / sizeof(core::data::geometry_t::index_size_t);
	if(instanceCount == 0)
	{
		glDrawElements(material->data().wireframe() ? GL_LINES : GL_TRIANGLES,
					   indices,
					   GL_UNSIGNED_INT,
					   (void*)(m_IndicesSubRange.begin + m_IndicesSegment.range().begin));
	}
	else
	{
		glDrawElementsInstanced(material->data().wireframe() ? GL_LINES : GL_TRIANGLES,
								indices,
								GL_UNSIGNED_INT,
								(void*)(m_IndicesSubRange.begin + m_IndicesSegment.range().begin),
								instanceCount);
	}


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
};

bool geometry_t::compatible(const core::igles::material_t& material) const noexcept
{
	for(const auto& stage : material.data().stages())
	{
		for(const auto& attribute : stage.attributes())
		{
			if(!attribute.input_rate() || attribute.input_rate() != core::gfx::vertex_input_rate::vertex) continue;

			for(const auto& b : m_Bindings)
			{
				if(psl::to_string8_t(b.name) == attribute.tag())
				{
					goto success;
				}
			}
			goto error;

		success:
			continue;

		error:
			core::igles::log->error(
			  "missing ATTRIBUTE [{0}] in GEOMETRY [{1}]", attribute.tag(), utility::to_string(m_UID));
			return false;
		}
	}
	return true;
}


size_t geometry_t::vertices() const noexcept { return m_Vertices; }
size_t geometry_t::indices() const noexcept { return m_Triangles * 3; }
size_t geometry_t::triangles() const noexcept { return m_Triangles; }