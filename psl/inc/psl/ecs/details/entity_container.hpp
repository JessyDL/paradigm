#pragma once
#include "psl/array.hpp"
#include "psl/ecs/entity.hpp"
#include "psl/serialization/serializer.hpp"
#include "psl/sparse_array.hpp"
#include "psl/utility/cast.hpp"

namespace psl::ecs::details {
class entity_container_t {
  protected:
	entity_container_t();
	entity_container_t(entity_container_t const&)			 = delete;
	entity_container_t(entity_container_t&&)				 = delete;
	entity_container_t& operator=(entity_container_t const&) = delete;
	entity_container_t& operator=(entity_container_t&&)		 = delete;

	template <typename S>
	void serialize(S& serializer) {
		if constexpr(psl::serialization::details::IsDecoder<S>) {
			if(size() != 0 || modified_entities_size() != 0) {
				throw std::runtime_error("unsupported deserializing into non-empty state");
			}
		}

		serializer.template parse<"ORPHANS">(m_Orphans);
		serializer.template parse<"FUTURE_ORPHANS">(m_ToBeOrphans);
		serializer.template parse<"ENTITIES">(m_Entities);
	}

  public:
	[[maybe_unused]] entity_t create();
	[[maybe_unused]] psl::array<entity_t> create(auto count) {
		return create(psl::narrow_cast<entity_t::size_type>(count));
	}
	[[maybe_unused]] psl::array<entity_t> create(entity_t::size_type count);

	void destroy(psl::array_view<entity_t> entities) noexcept;
	void destroy(entity_t entity) noexcept;

	psl::array<entity_t> all_entities() const noexcept;
	size_t capacity() const noexcept {
		return m_Entities;
	}

	size_t size() const noexcept {
		return m_Entities - m_Orphans.size();
	}

  protected:
	void process_to_be_orphans() noexcept;
	size_t modified_entities_size() const noexcept {
		return m_ModifiedEntities.size();
	}
	void clear_modified_entities() noexcept {
		m_ModifiedEntities.clear();
	}
	[[nodiscard]] psl::array<entity_t> modified_entities() const noexcept {
		auto view = m_ModifiedEntities.indices();
		return psl::array<entity_t>(reinterpret_cast<entity_t const*>(view.data()),
									reinterpret_cast<entity_t const*>(view.data()) + view.size());
	}

	void modify_entities(psl::array_view<entity_t> entities) noexcept;
	void modify_entities(psl::array_view<entity_t::size_type> entities) noexcept;
	void modify_entity(entity_t entity) noexcept;
	void clear() noexcept;

  private:
	psl::array<entity_t> m_Orphans {};
	psl::array<entity_t> m_ToBeOrphans {};
	psl::sparse_indice_array<entity_t::size_type> m_ModifiedEntities {};
	entity_t::size_type m_Entities {0};
};
}	 // namespace psl::ecs::details
