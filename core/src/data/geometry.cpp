#include "core/data/geometry.hpp"
#include "psl/array_view.hpp"
#include "core/resource/resource.hpp"

using namespace psl;
using namespace core::data;
using namespace core::resource;

geometry_t::geometry_t(core::resource::cache_t& cache,
					   const core::resource::metadata& metaData,
					   psl::meta::file* metaFile) noexcept {}

geometry_t::geometry_t(core::resource::cache_t& cache,
					   const core::resource::metadata& metaData,
					   psl::meta::file* metaFile,
					   const geometry_t& other) noexcept {
	m_Indices		= other.m_Indices;
	m_VertexStreams = other.m_VertexStreams;
}

bool geometry_t::contains(psl::string_view name) const noexcept {
	return m_VertexStreams.value.contains(psl::string {name});
}

const core::vertex_stream_t& geometry_t::vertices(const psl::string_view name) const {
	auto it = m_VertexStreams.value.find(psl::string {name});
	psl_assert(
	  it != std::end(m_VertexStreams.value), "The given stream '{}' was not found within the geometry data", name);
	if(it != std::end(m_VertexStreams.value)) {
		return it->second;
	}
	throw std::runtime_error("Missing requested data stream in the geometry data");
}
void geometry_t::vertices(const psl::string_view name, const core::vertex_stream_t& stream) {
	m_VertexStreams.value[psl::string {name}] = stream;
}
const std::vector<geometry_t::index_size_t>& geometry_t::indices() const {
	return m_Indices.value;
}
void geometry_t::indices(psl::array_view<index_size_t> indices) {
	m_Indices.value = psl::array<index_size_t> {indices};
}

const std::unordered_map<psl::string, core::vertex_stream_t>& geometry_t::vertex_streams() const {
	return m_VertexStreams.value;
}

bool geometry_t::is_valid() const noexcept {
	auto it = m_VertexStreams.value.find(psl::string {constants::POSITION});
	if(it == std::end(m_VertexStreams.value)) {
		return false;
	}

	return !std::any_of(std::begin(m_VertexStreams.value), std::end(m_VertexStreams.value), [&it](const auto& pair) {
		return pair.second.size() != it->second.size();
	});
}

size_t geometry_t::bytesize() const noexcept {
	return std::accumulate(std::begin(m_VertexStreams.value),
						   std::end(m_VertexStreams.value),
						   (size_t)0,
						   [](auto sum, const auto& pair) { return sum + pair.second.bytesize(); });
}


size_t geometry_t::elements() const noexcept {
	return std::accumulate(std::begin(m_VertexStreams.value),
						   std::end(m_VertexStreams.value),
						   (size_t)0,
						   [](auto sum, const auto& pair) { return sum + pair.second.elements(); });
}

bool geometry_t::erase(psl::string_view name) noexcept {
	return m_VertexStreams.value.erase(psl::string(name)) > 0;
}


geometry_t::index_size_t geometry_t::vertex_count() const noexcept {
	return static_cast<index_size_t>(
	  (m_VertexStreams.value.size() == 0) ? 0 : std::begin(m_VertexStreams.value)->second.size());
}
geometry_t::index_size_t geometry_t::index_count() const noexcept {
	return static_cast<index_size_t>(m_Indices.value.size());
}
geometry_t::index_size_t geometry_t::triangles() const noexcept {
	return static_cast<index_size_t>(m_Indices.value.size() / 3);
}
