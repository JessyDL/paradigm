#include "data/geometry.h"
#include "resource/resource.hpp"
#include "psl/array_view.h"

using namespace psl;
using namespace core::data;
using namespace core::resource;

geometry::geometry(core::resource::cache& cache, const core::resource::metadata& metaData,
				   psl::meta::file* metaFile) noexcept
{}


std::optional<std::reference_wrapper<const core::stream>> geometry::vertices(const psl::string_view name) const
{
	auto it = m_VertexStreams.value.find(psl::string{name});
	if(it != std::end(m_VertexStreams.value))
	{
		return it->second;
	}
	return std::nullopt;
}
void geometry::vertices(const psl::string_view name, const core::stream& stream)
{
	m_VertexStreams.value[psl::string{name}] = stream;
}
const std::vector<geometry::index_size_t>& geometry::indices() const { return m_Indices.value; }
void geometry::indices(psl::array_view<index_size_t> indices) { m_Indices.value = psl::array<index_size_t>{ indices }; }

const std::unordered_map<psl::string, core::stream>& geometry::vertex_streams() const { return m_VertexStreams.value; }

bool geometry::is_valid() const noexcept
{
	auto it = m_VertexStreams.value.find(psl::string{constants::POSITION});
	if(it == std::end(m_VertexStreams.value))
	{
		return false;
	}

	return !std::any_of(std::begin(m_VertexStreams.value), std::end(m_VertexStreams.value),
						[&it](const auto& pair) { return pair.second.size() != it->second.size(); });
}

size_t geometry::bytesize() const noexcept
{
	return std::accumulate(std::begin(m_VertexStreams.value), std::end(m_VertexStreams.value), (size_t)0,
		[](auto sum, const auto& pair) { return sum + pair.second.bytesize(); });
}


size_t geometry::elements() const noexcept
{
	return std::accumulate(std::begin(m_VertexStreams.value), std::end(m_VertexStreams.value), (size_t)0,
		[](auto sum, const auto& pair) { return sum + pair.second.elements(); });

}

bool geometry::erase(psl::string_view name) noexcept
{
	return m_VertexStreams.value.erase(psl::string(name)) > 0;
}


geometry::index_size_t geometry::vertex_count() const noexcept
{
	return static_cast<index_size_t>((m_VertexStreams.value.size() == 0) ? 0 : std::begin(m_VertexStreams.value)->second.size());
}
geometry::index_size_t geometry::index_count() const noexcept
{
	return static_cast<index_size_t>(m_Indices.value.size());
}
geometry::index_size_t geometry::triangles() const noexcept
{
	return static_cast<index_size_t>(m_Indices.value.size()/3);
}