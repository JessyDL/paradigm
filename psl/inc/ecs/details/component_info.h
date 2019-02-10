#pragma once
#include "vector.h"
#include "unordered_map.h"
#include "component_key.h"
#include "IDGenerator.h"
#include "array_view.h"

namespace memory
{
	class raw_region;
}
namespace psl::ecs
{
	struct entity;
}

namespace psl
{
	template <typename... Ts>
	class pack_view;
}

namespace psl::ecs::details
{
	class component_info
	{
	  public:
		component_info() = default;
		component_info(component_key_t id, size_t size, size_t capacity = 256);

		component_info(const component_info& other) = delete;
		component_info(component_info&& other);
		~component_info();
		component_info& operator=(const component_info& other) = delete;
		component_info& operator=(component_info&& other);
		void grow();

		void shrink();

		void resize(size_t new_capacity);

		uint64_t available() const noexcept;

		bool is_tag() const noexcept;


		psl::array<std::pair<uint64_t, uint64_t>> add(psl::array_view<entity> entities);
		void destroy(psl::array_view<entity> entities, psl::array_view<uint64_t> indices);
		void purge();

		psl::array_view<entity> entities() const noexcept;
		psl::array_view<entity> deleted_entities() const noexcept;

		void* data() const noexcept;

		size_t id() const noexcept;
	  private:
		psl::array<entity> m_Entities;
		psl::array<entity> m_DeletedEntities;
		psl::array<std::pair<uint64_t, uint64_t>> m_MarkedForDeletion;
		memory::raw_region* m_Region;
		psl::generator<uint64_t> m_Generator;
		component_key_t m_ID;
		size_t m_Capacity;
		size_t m_Size;
	};
} // namespace psl::ecs::details

namespace std
{
	template <>
	struct hash<psl::ecs::details::component_info>
	{
		std::size_t operator()(psl::ecs::details::component_info const& ci) const noexcept
		{
			return ci.id();
		}
	};
} // namespace std