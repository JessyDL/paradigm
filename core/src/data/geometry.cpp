#include "data/geometry.h"
#include "resource/resource.hpp"

using namespace psl;
using namespace core::data;
using namespace core::resource;

geometry::geometry(const UID& uid, core::resource::cache& cache) {}
geometry::~geometry() {}


std::optional<std::reference_wrapper<const core::stream>> 
geometry::vertices(const psl::string_view name) const
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
const std::vector<geometry::index_size_t>& geometry::indices() const { return m_Indices.value;
	}
void geometry::indices(const std::vector<index_size_t>& indices) { m_Indices = indices; }

const std::unordered_map<psl::string, core::stream>& geometry::vertex_streams() const {
	return m_VertexStreams.value;
}

bool geometry::is_valid() const 
{ 
	auto it					 = m_VertexStreams.value.find(psl::string{constants::POSITION});
	if(it == std::end(m_VertexStreams.value))
	{
		return false;
	}

	return !std::any_of(std::begin(m_VertexStreams.value), std::end(m_VertexStreams.value), [&it](const auto& pair){return pair.second.size() != it->second.size();});
}
