#pragma once
#include "entity.h"
#include "selectors.h"
#include "pack.h"
#include "IDGenerator.h"
#include "vector.h"
#include "details/component_key.h"
#include "details/entity_info.h"
#include "details/component_info.h"
#include "unique_ptr.h"
#include "bytell_map.h"
#include "array.h"
#include <chrono>
#include "task.h"
#include "template_utils.h"
#include <functional>

namespace psl::async
{
	class scheduler;
}

namespace psl::ecs
{
	enum class threading
	{
		seq		   = 0,
		sequential = seq,
		par		   = 1,
		parallel   = par,
		main	   = 2
	};

	template <typename T>
	struct empty
	{};

	struct storage
	{
		psl::bytell_map<entity, details::entity_info> entities;
		psl::bytell_map<details::component_key_t, details::component_info> components;
	};


	class state
	{
	  public:
		state(size_t workers = 0);
		~state()			= default;
		state(const state&) = delete;
		state(state&&)		= delete;
		state& operator=(const state&) = delete;
		state& operator=(state&&) = delete;

		template <typename... Ts>
		void add_components(psl::array_view<entity> entities)
		{
			(add_component<Ts>(entities), ...);
		}
		template <typename... Ts>
		void add_components(psl::array_view<entity> entities, Ts&&... prototype)
		{
			(add_component(entities, std::forward<Ts>(prototype)), ...);
		}

		template <typename... Ts>
		void remove_components(psl::array_view<entity> entities) noexcept
		{
			(remove_component(details::key_for<Ts>(), entities), ...);
		}

		template <typename... Ts>
		void add_components(psl::array_view<std::pair<entity, entity>> entities)
		{
			(add_component<Ts>(entities), ...);
		}
		template <typename... Ts>
		void add_components(psl::array_view<std::pair<entity, entity>> entities, Ts&&... prototype)
		{
			(add_component(entities, std::forward<Ts>(prototype)), ...);
		}

		template <typename... Ts>
		void remove_components(psl::array_view<std::pair<entity, entity>> entities) noexcept
		{
			(remove_component(details::key_for<Ts>(), entities), ...);
		}

		template <typename... Ts>
		psl::ecs::pack<Ts...> get_components(psl::array_view<entity> entities) const noexcept;


		template <typename... Ts>
		bool has_components(psl::array_view<entity> entities) const noexcept;

		template <typename T, typename... Ts>
		bool is_owned_by(entity e, const T& component, const Ts&... components) const noexcept;

		template <typename... Ts>
		psl::array<std::pair<entity, entity>> create(entity count = 1)
		{
			if(count > 1)
			{
				auto entity_ranges = m_Generator.create_multi(count);
				if constexpr(sizeof...(Ts) > 0)
				{
					( add_components<Ts>(entity_ranges), ...);
				}
				return entity_ranges;
			}
			else
			{
				auto id = m_Generator.create(count);
				psl::array<std::pair<entity, entity>> entity_ranges{{id, id + 1}};
				if constexpr(sizeof...(Ts) > 0)
				{
					(add_components<Ts>(entity_ranges), ...);
				}
				return entity_ranges;
			}
		}

		template <typename... Ts>
		psl::array<std::pair<entity, entity>> create(entity count, Ts&&... prototype)
		{
			auto entity_ranges = m_Generator.create_multi(count);

			add_components(entity_ranges, std::forward<Ts>(prototype)...);

			return entity_ranges;
		}

		void destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept;
		void destroy(entity entity) noexcept;

		template <typename... Ts>
		psl::array<entity> filter() const noexcept
		{
			std::optional<psl::array<entity>> entities{std::nullopt};
			(void(entities = filter_impl(psl::templates::proxy_type<Ts>{}, entities)), ...);

			return (entities) ? entities.value() : psl::array<entity>{};
		}


		template <typename... Ts>
		psl::array<entity> filter(psl::array_view<entity> entities) const noexcept
		{
			psl::array<entity> result{entities};
			(void(result = filter_impl(psl::templates::proxy_type<Ts>{}, result)), ...);

			return result;
		}

		template <typename... Ts>
		void set(psl::array_view<entity> entities, psl::array_view<Ts>... data) noexcept;

		template <typename... Ts>
		void set_or_create(psl::array_view<entity> entities, psl::array_view<Ts>... data) noexcept;

		void tick(std::chrono::duration<float> dTime);

		void reset(psl::array_view<entity> entities) noexcept;
		
	  private:
		//------------------------------------------------------------
		// helpers
		//------------------------------------------------------------
		details::component_info& get_component_info(details::component_key_t key, size_t size);

		//------------------------------------------------------------
		// add_component
		//------------------------------------------------------------
		template <typename T>
		void add_component(psl::array_view<std::pair<entity, entity>> entities, T&& prototype)
		{
			if constexpr(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value &&
						 std::is_trivially_destructible<T>::value)
			{
				static_assert(!std::is_empty_v<T>,
							  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
							  "psl::ecs::empty<T>{} to avoid initialization.");
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &prototype);
			}
			else if constexpr(std::is_constructible<decltype(std::function(prototype)), T>::value)
			{
				using tuple_type = typename psl::templates::func_traits<T>::arguments_t;
				static_assert(std::tuple_size<tuple_type>::value == 1,
							  "only one argument is allowed in the prototype invocable");
				using arg0_t = std::tuple_element_t<0, tuple_type>;
				static_assert(std::is_reference_v<arg0_t> && !std::is_const_v<arg0_t>,
							  "the argument type should be of 'T&'");
				using type = typename std::remove_reference<arg0_t>::type;
				static_assert(!std::is_empty_v<type>,
							  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
							  "psl::ecs::empty<T>{} to avoid initialization.");
				add_component_impl(details::key_for<type>(), entities, sizeof(type),
								   [prototype](std::uintptr_t location, size_t count) {
									   for(auto i = size_t{0}; i < count; ++i)
									   {
										   std::invoke(prototype, *((type*)(location) + i));
									   }
								   });
			}
			else
			{
				static_assert(psl::templates::always_false<T>::value,
							  "could not figure out if the template type was an invocable or a component prototype");
			}
		}

		template <typename T>
		void add_component(psl::array_view<std::pair<entity, entity>> entities, psl::ecs::empty<T>&& prototype)
		{
			if constexpr(std::is_trivially_constructible_v<T>)
			{
				add_component_impl(details::key_for<T>(), entities, (std::is_empty<T>::value) ? 0 : sizeof(T));
			}
			else
			{
				T v{};
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &v);
			}
		}

		template <typename T>
		void add_component(psl::array_view<std::pair<entity, entity>> entities)
		{
			if constexpr(std::is_trivially_constructible_v<T>)
			{
				add_component_impl(details::key_for<T>(), entities, (std::is_empty<T>::value) ? 0 : sizeof(T));
			}
			else
			{
				T v{};
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &v);
			}
		}

		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities, size_t size);
		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities, size_t size,
								std::function<void(std::uintptr_t, size_t)> invocable);
		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities, size_t size,
								void* prototype);

		//------------------------------------------------------------
		// remove_component
		//------------------------------------------------------------
		void remove_component(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities) noexcept;


		//------------------------------------------------------------
		// filter
		//------------------------------------------------------------

		template <typename T>
		psl::array<entity> filter_impl(psl::templates::proxy_type<T>,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept
		{
			return filter_impl(psl::templates::proxy_type<psl::ecs::filter<T>>(), entities);
		}

		template <typename T>
		psl::array<entity> filter_impl(psl::templates::proxy_type<psl::ecs::filter<T>>,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept
		{
			return filter_impl(details::key_for<T>(), entities);
		}
		template <typename T>
		psl::array<entity> filter_impl(psl::templates::proxy_type<psl::ecs::on_add<T>>,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept
		{
			return filter_impl(details::key_for<T>(), m_Changes[m_Tick % 2].added_components, entities);
		}
		template <typename T>
		psl::array<entity> filter_impl(psl::templates::proxy_type<psl::ecs::on_remove<T>>,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept
		{
			return filter_impl(details::key_for<T>(), m_Changes[m_Tick % 2].removed_components, entities);
		}
		template <typename T>
		psl::array<entity> filter_impl(psl::templates::proxy_type<psl::ecs::except<T>>,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept
		{
			return filter_except_impl(psl::templates::proxy_type<psl::ecs::filter<T>>(), entities);
		}
		template <typename... Ts>
		psl::array<entity> filter_impl(psl::templates::proxy_type<psl::ecs::on_break<Ts...>>,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept
		{}
		template <typename... Ts>
		psl::array<entity> filter_impl(psl::templates::proxy_type<psl::ecs::on_combine<Ts...>>,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept
		{}

		psl::array<entity> filter_impl(details::component_key_t key,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept;
		psl::array<entity> filter_except_impl(details::component_key_t key,
											  std::optional<psl::array_view<entity>> entities = std::nullopt) const
			noexcept;

		psl::array<entity> filter_impl(details::component_key_t key,
									   const psl::bytell_map<details::component_key_t, psl::array<entity>>& map,
									   std::optional<psl::array_view<entity>> entities = std::nullopt) const noexcept;
		//------------------------------------------------------------
		// set
		//------------------------------------------------------------
		void set(psl::array_view<entity> entities, details::component_key_t key, void* data) noexcept;


		struct difference_set
		{
			psl::bytell_map<details::component_key_t, psl::array<std::pair<entity, entity>>> added_components;
			psl::bytell_map<details::component_key_t, psl::array<std::pair<entity, entity>>> removed_components;


			psl::bytell_map<entity, details::entity_info> destroyed_data;
		};


		psl::array<std::pair< details::component_key_t, details::component_info>> m_Components;
		//psl::bytell_map<details::component_key_t, details::component_info> m_Components;

		psl::generator<entity> m_Generator;
		psl::unique_ptr<psl::async::scheduler> m_Scheduler;

		psl::static_array<difference_set, 2> m_Changes;
		size_t m_Tick{0};
		size_t m_ChangeSetTick{0};
	};
} // namespace psl::ecs