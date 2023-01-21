#include "core/data/bundle.hpp"
#include "core/systems/resource.hpp"

using namespace core::data;
using namespace core::resource;

bundle::bundle(const psl::UID& uid, core::resource::cache_t& cache) noexcept {};
psl::array_view<bundle::data> bundle::materials() const noexcept {
	return m_Data.value;
}
void bundle::add(psl::array_view<std::pair<psl::UID, uint32_t>> materials) {
	std::transform(std::begin(materials),
				   std::end(materials),
				   std::back_inserter(m_Data.value),
				   [](const std::pair<psl::UID, uint32_t>& value) {
					   return bundle::data {value.first, value.second};
				   });
}
void bundle::remove(psl::UID material) noexcept {
	m_Data.value.erase(std::remove_if(std::begin(m_Data.value),
									  std::end(m_Data.value),
									  [&material](const bundle::data& data) { return data.material() == material; }),
					   std::end(m_Data.value));
}
void bundle::remove(psl::UID material, uint32_t layer) noexcept {
	m_Data.value.erase(std::remove_if(std::begin(m_Data.value),
									  std::end(m_Data.value),
									  [&material, layer](const bundle::data& data) {
										  return data.material() == material && data.layer() == layer;
									  }),
					   std::end(m_Data.value));
}
void bundle::remove(uint32_t layer) noexcept {
	m_Data.value.erase(std::remove_if(std::begin(m_Data.value),
									  std::end(m_Data.value),
									  [layer](const bundle::data& data) { return data.layer() == layer; }),
					   std::end(m_Data.value));
}

bundle::data::data(const psl::UID& material, uint32_t layer) noexcept : m_Material(material), m_Layer(layer) {}
const psl::UID& bundle::data::material_t() const noexcept {
	return m_Material.value;
}
void bundle::data::material_t(const psl::UID& value) noexcept {
	m_Material.value = value;
}
uint32_t bundle::data::layer() const noexcept {
	return m_Layer.value;
}
void bundle::data::layer(uint32_t value) noexcept {
	m_Layer.value = value;
}
