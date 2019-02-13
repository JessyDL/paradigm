#pragma once
#include "vector.h"
#include "unordered_map.h"
#include "component_key.h"
#include "IDGenerator.h"
#include "array_view.h"
#include "../entity.h"
#include "sparse_array.h"

namespace memory
{
	class raw_region;
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


		psl::array<std::pair<entity, entity>> add(psl::array_view<std::pair<entity, entity>> entities);
		void destroy(psl::array_view<std::pair<entity, entity>> entities);
		void destroy(entity entity);
		void purge();

		void* data() const noexcept;

		size_t id() const noexcept;

		psl::array_view<entity> entities() const noexcept;
		bool has_component(entity e) const noexcept;
	  private:
		psl::sparse_array<entity, entity> m_Entities;

		memory::raw_region* m_Region;
		psl::generator<entity> m_Generator;
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