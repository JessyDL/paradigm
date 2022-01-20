#include "data/geometry.h"
#include "psl/array_view.hpp"
#include "resource/resource.hpp"

using namespace psl;
using namespace core::data;
using namespace core::resource;

geometry_t::geometry_t(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile) noexcept
{}


std::optional<std::reference_wrapper<const core::stream>> geometry_t::vertices(const psl::string_view name) const
{
	auto it = m_VertexStreams.value.find(psl::string {name});
	if(it != std::end(m_VertexStreams.value))
	{
		return it->second;
	}
	return std::nullopt;
}
void geometry_t::vertices(const psl::string_view name, const core::stream& stream)
{
	m_VertexStreams.value[psl::string {name}] = stream;
}
const std::vector<geometry_t::index_size_t>& geometry_t::indices() const { return m_Indices.value; }
void geometry_t::indices(psl::array_view<index_size_t> indices) { m_Indices.value = psl::array<index_size_t> {indices}; }

const std::unordered_map<psl::string, core::stream>& geometry_t::vertex_streams() const { return m_VertexStreams.value; }

bool geometry_t::is_valid() const noexcept
{
	auto it = m_VertexStreams.value.find(psl::string {constants::POSITION});
	if(it == std::end(m_VertexStreams.value))
	{
		return false;
	}

	return !std::any_of(std::begin(m_VertexStreams.value), std::end(m_VertexStreams.value), [&it](const auto& pair) {
		return pair.second.size() != it->second.size();
	});
}

size_t geometry_t::bytesize() const noexcept
{
	return std::accumulate(std::begin(m_VertexStreams.value),
						   std::end(m_VertexStreams.value),
						   (size_t)0,
						   [](auto sum, const auto& pair) { return sum + pair.second.bytesize(); });
}


size_t geometry_t::elements() const noexcept
{
	return std::accumulate(std::begin(m_VertexStreams.value),
						   std::end(m_VertexStreams.value),
						   (size_t)0,
						   [](auto sum, const auto& pair) { return sum + pair.second.elements(); });
}

bool geometry_t::erase(psl::string_view name) noexcept { return m_VertexStreams.value.erase(psl::string(name)) > 0; }


geometry_t::index_size_t geometry_t::vertex_count() const noexcept
{
	return static_cast<index_size_t>(
	  (m_VertexStreams.value.size() == 0) ? 0 : std::begin(m_VertexStreams.value)->second.size());
}
geometry_t::index_size_t geometry_t::index_count() const noexcept
{
	return static_cast<index_size_t>(m_Indices.value.size());
}
geometry_t::index_size_t geometry_t::triangles() const noexcept
{
	return static_cast<index_size_t>(m_Indices.value.size() / 3);
}