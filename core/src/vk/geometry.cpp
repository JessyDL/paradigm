#include "vk/geometry.hpp"
#include "data/buffer.hpp"
#include "data/geometry.hpp"
#include "data/material.hpp"
#include "gfx/types.hpp"
#include "logging.hpp"
#include "meta/shader.hpp"
#include "resource/resource.hpp"
#include "vk/buffer.hpp"
#include "vk/material.hpp"
#include "vk/shader.hpp"

using namespace psl;
using namespace core::resource;
using namespace core;
using namespace core::ivk;

constexpr const vk::IndexType INDEX_TYPE =
  sizeof(core::data::geometry_t::index_size_t) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;

geometry_t::geometry_t(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   handle<core::ivk::context> context,
				   core::resource::handle<core::data::geometry_t> data,
				   core::resource::handle<core::ivk::buffer_t> geometryBuffer,
				   core::resource::handle<core::ivk::buffer_t> indicesBuffer) :
	m_Context(context),
	m_Data(data), m_GeometryBuffer(geometryBuffer), m_IndicesBuffer(indicesBuffer), m_UID(metaData.uid)
{
	recreate(m_Data);
}

geometry_t::~geometry_t()
{
	if(!m_GeometryBuffer) return;

	clear();
}

void geometry_t::clear()
{
	for(auto& binding : m_Bindings)
	{
		// this check makes sure this is the owner of the memory segment. if there's a local offset it means this
		// binding has a shared memory::segment and we should only clear the first one who coincidentally starts at
		// begin 0
		if(binding.sub_range.begin == 0) m_GeometryBuffer->deallocate(binding.segment);
	}
	m_Bindings.clear();
	// same as the earlier comment for geometry buffer
	if(m_IndicesBuffer && m_IndicesSubRange.begin == 0 && m_IndicesSubRange.size() > 0)
		m_IndicesBuffer->deallocate(m_IndicesSegment);

	m_Data = {};
}
void geometry_t::recreate(core::resource::handle<core::data::geometry_t> data)
{
	clear();
	std::vector<vk::DeviceSize> sizeRequests;
	sizeRequests.reserve(data->vertex_streams().size() + ((m_GeometryBuffer == m_IndicesBuffer) ? 1 : 0));
	std::for_each(std::begin(data->vertex_streams()),
				  std::end(data->vertex_streams()),
				  [&sizeRequests](const std::pair<psl::string, core::stream>& element) {
					  sizeRequests.emplace_back((uint32_t)element.second.bytesize());
				  });

	if(m_GeometryBuffer == m_IndicesBuffer)
	{
		sizeRequests.emplace_back((uint32_t)(data->indices().size() * sizeof(core::data::geometry_t::index_size_t)));
	}

	auto segments = m_GeometryBuffer->reserve(sizeRequests, true);
	if(segments.size() == 0)
	{
		core::ivk::log->critical("ran out of memory, could not allocate enough in the buffer to accomodate");
		exit(1);
	}
	assert(data->vertex_streams().size() > 0);
	m_Vertices = std::begin(data->vertex_streams())->second.size();
	std::vector<core::gfx::commit_instruction> instructions;
	size_t i = 0;
	for(const auto& stream : data->vertex_streams())
	{
		assert(m_Vertices == stream.second.size());
		auto& instr	  = instructions.emplace_back();
		instr.size	  = stream.second.bytesize();
		instr.source  = (std::uintptr_t)(stream.second.cdata());
		instr.segment = segments[i].first;
		if(segments[i].first.range() != segments[i].second) instr.sub_range = segments[i].second;

		auto& b		= m_Bindings.emplace_back();
		b.name		= stream.first;
		b.segment	= segments[i].first;
		b.sub_range = segments[i].second;
		++i;
	}

	if(m_GeometryBuffer != m_IndicesBuffer)
	{
		if(auto indiceSegment =
			 m_IndicesBuffer->reserve((uint32_t)(data->indices().size() * sizeof(core::data::geometry_t::index_size_t)));
		   indiceSegment)
		{
			m_IndicesSegment  = indiceSegment.value();
			m_IndicesSubRange = memory::range_t {0, m_IndicesSegment.range().size()};
		}
		else
		{
			core::ivk::log->critical("index buffer was out of memory");
			// todo error condition could not allocate segment
			exit(1);
		}
		gfx::commit_instruction instr;
		instr.size	  = m_IndicesSegment.range().size();
		instr.source  = (std::uintptr_t)data->indices().data();
		instr.segment = m_IndicesSegment;
		m_IndicesBuffer->commit({instr});
	}
	else
	{
		i			  = segments.size() - 1;
		auto& instr	  = instructions.emplace_back();
		instr.size	  = segments[i].second.size();
		instr.source  = (std::uintptr_t)data->indices().data();
		instr.segment = segments[i].first;
		if(segments[i].first.range() != segments[i].second) instr.sub_range = segments[i].second;

		m_IndicesSegment  = segments[i].first;
		m_IndicesSubRange = segments[i].second;
	}

	m_Triangles = (m_IndicesSubRange.size() / sizeof(core::data::geometry_t::index_size_t)) / 3u;

	m_GeometryBuffer->commit(instructions);
	m_Data = data;
}
void geometry_t::recreate(core::resource::handle<core::data::geometry_t> data,
						core::resource::handle<core::ivk::buffer_t> geometryBuffer,
						core::resource::handle<core::ivk::buffer_t> indicesBuffer)
{
	clear();

	m_GeometryBuffer = geometryBuffer;
	m_IndicesBuffer	 = indicesBuffer;

	recreate(data);
}


bool geometry_t::compatible(const core::ivk::material_t& material) const noexcept
{
	for(const auto& stage : material.data()->stages())
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
			core::ivk::log->error(
			  "missing ATTRIBUTE [{0}] in GEOMETRY [{1}]", attribute.tag(), utility::to_string(m_UID));
			return false;
		}
	}
	return true;
}

void geometry_t::bind(vk::CommandBuffer& buffer, const core::ivk::material_t& material) const noexcept
{
	for(const auto& stage : material.data()->stages())
	{
		for(const auto& attribute : stage.attributes())
		{
			if(!attribute.input_rate() || attribute.input_rate() != core::gfx::vertex_input_rate::vertex) continue;

			auto binding = std::find_if(
			  std::begin(m_Bindings), std::end(m_Bindings), [tag = attribute.tag()](const auto& binding) noexcept {
				  return binding.name == tag.data();
			  });

			auto offset = vk::DeviceSize {binding->segment.range().begin + binding->sub_range.begin};
			buffer.bindVertexBuffers(attribute.location(), 1, &m_GeometryBuffer->gpu_buffer(), &offset);
		}
	}

	buffer.bindIndexBuffer(
	  m_IndicesBuffer->gpu_buffer(), m_IndicesSegment.range().begin + m_IndicesSubRange.begin, INDEX_TYPE);
}


size_t geometry_t::vertices() const noexcept { return m_Vertices; }
size_t geometry_t::triangles() const noexcept { return m_Vertices; }
size_t geometry_t::indices() const noexcept { return m_Vertices * 3; }