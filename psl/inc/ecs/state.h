#pragma once
#include "entity.h"
#include "selectors.h"
#include "pack.h"
#include "command_buffer.h"
#include "IDGenerator.h"
#include "vector.h"
#include "details/component_key.h"
#include "details/entity_info.h"
#include "details/component_info.h"
#include "details/system_information.h"
#include "unique_ptr.h"
#include "bytell_map.h"
#include "array.h"
#include <chrono>
#include "task.h"
#include "template_utils.h"
#include <functional>
#include "memory/raw_region.h"

namespace psl::async
{
	class scheduler;
}

namespace psl::ecs
{

	template <typename T>
	struct empty
	{};

	class state final
	{
	  public:
		state(size_t workers = 0);
		~state()			= default;
		state(const state&) = delete;
		state(state&&)		= default;
		state& operator=(const state&) = delete;
		state& operator=(state&&) = default;

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

		template <typename T>
		T& get(entity entity)
		{
			auto cInfo = get_component_typed_info<T>();
			return cInfo->entity_data()[entity];
		}

		template <typename... Ts>
		bool has_components(psl::array_view<entity> entities) const noexcept
		{
			for(const auto& cInfo : m_Components)
			{
				return std::all_of(std::begin(entities), std::end(entities),
								   [&cInfo](entity e) { return cInfo->has_component(e); });
			}
		}

		template <typename T, typename... Ts>
		bool is_owned_by(entity e, const T& component, const Ts&... components) const noexcept;

		entity create()
		{
			if(m_Orphans > 0)
			{
				--m_Orphans;
				const auto orphan		   = m_Next;
				m_Next					   = m_Entities[(size_t)m_Next];
				m_Entities[(size_t)orphan] = orphan;
				return orphan;
			}
			else
			{
				// we do this to protect ourselves from 1 entity loops
				if(m_Entities.size() + 1 >= m_Entities.capacity()) m_Entities.reserve(m_Entities.size() * 2 + 1);

				return m_Entities.emplace_back((entity)m_Entities.size());
			}
		}

		template <typename... Ts>
		psl::array<entity> create(entity count)
		{
			psl::array<entity> entities;
			const auto recycled = std::min<entity>(count, (entity)m_Orphans);
			m_Orphans -= recycled;
			const auto remainder = count - recycled;

			entities.reserve(recycled + remainder);
			// we do this to protect ourselves from 1 entity loops
			if(m_Entities.size() + remainder >= m_Entities.capacity())
				m_Entities.reserve(m_Entities.size() * 2 + remainder);

			for(size_t i = 0; i < recycled; ++i)
			{
				const auto orphan = m_Next;
				entities.emplace_back(orphan);
				m_Next					   = m_Entities[(size_t)m_Next];
				m_Entities[(size_t)orphan] = orphan;
			}

			for(size_t i = 0; i < remainder; ++i)
			{
				entities.emplace_back((entity)m_Entities.size());
				m_Entities.emplace_back((entity)m_Entities.size());
			}

			if constexpr(sizeof...(Ts) > 0)
			{
				(add_components<Ts>(entities), ...);
			}
			return entities;
		}

		template <typename... Ts>
		psl::array<entity> create(entity count, Ts&&... prototype)
		{
			psl::array<entity> entities;
			const auto recycled = std::min<entity>(count, (entity)m_Orphans);
			m_Orphans -= recycled;
			const auto remainder = count - recycled;

			entities.reserve(recycled + remainder);
			// we do this to protect ourselves from 1 entity loops
			if(m_Entities.size() + remainder >= m_Entities.capacity())
				m_Entities.reserve(m_Entities.size() * 2 + remainder);
			for(size_t i = 0; i < recycled; ++i)
			{
				auto orphan = m_Next;
				entities.emplace_back(orphan);
				m_Next					   = m_Entities[(size_t)m_Next];
				m_Entities[(size_t)orphan] = orphan;
			}

			for(size_t i = 0; i < remainder; ++i)
			{
				entities.emplace_back((entity)m_Entities.size());
				m_Entities.emplace_back((entity)m_Entities.size());
			}
			add_components(entities, std::forward<Ts>(prototype)...);

			return entities;
		}

		void destroy(psl::array_view<std::pair<entity, entity>> entities) noexcept;
		void destroy(psl::array_view<entity> entities) noexcept;
		void destroy(entity entity) noexcept;

		template <typename T, typename... Ts>
		psl::array<entity> filter() const noexcept
		{
			psl::array<entity> storage(filter_seed(psl::templates::proxy_type<T>{}));
			auto begin = std::begin(storage);
			auto end   = std::end(storage);
			if constexpr(sizeof...(Ts) == 0)
			{
				return storage;
			}
			else
			{
				(void(end = filter_remove(psl::templates::proxy_type<Ts>{}, begin, end)), ...);
				storage.erase(end, std::end(storage));
				return storage;
			}
		}
		template <typename... Ts>
		void set(psl::array_view<entity> entities, psl::array_view<Ts>... data) noexcept;

		template <typename... Ts>
		void set_or_create(psl::array_view<entity> entities, psl::array_view<Ts>... data) noexcept;

		void tick(std::chrono::duration<float> dTime);

		void reset(psl::array_view<entity> entities) noexcept;

		template <typename T>
		psl::array_view<entity> entities() const noexcept
		{
			auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [](const auto& cInfo) {
				constexpr auto key{details::key_for<T>()};
				return cInfo->id() == key;
			});
			return (it != std::end(m_Components)) ? (*it)->entities() : psl::array_view<entity>{};
		}

		template <typename T>
		psl::array_view<T> view()
		{
			auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [](const auto& cInfo) {
				constexpr auto key{details::key_for<T>()};
				return cInfo->id() == key;
			});
			return (it != std::end(m_Components))
					   ? ((details::component_info_typed<T>*)(it->operator->()))->entity_data().dense()
					   : psl::array_view<T>{};
		}

		template <typename Fn>
		void declare(Fn&& fn)
		{
			declare_impl(threading::sequential, std::forward<Fn>(fn));
		}


		template <typename Fn>
		void declare(threading threading, Fn&& fn)
		{
			declare_impl(threading, std::forward<Fn>(fn));
		}

		template <typename Fn, typename T>
		void declare(Fn&& fn, T* ptr)
		{
			declare_impl(threading::sequential, std::forward<Fn>(fn), ptr);
		}
		template <typename Fn, typename T>
		void declare(threading threading, Fn&& fn, T* ptr)
		{
			declare_impl(threading, std::forward<Fn>(fn), ptr);
		}

	  private:
		//------------------------------------------------------------
		// helpers
		//------------------------------------------------------------
		void fill_in(details::component_key_t key, psl::array_view<entity> entities,
					 psl::array_view<std::uintptr_t>& data);

		size_t prepare_bindings(psl::array_view<entity> entities, void* cache,
								details::dependency_pack& dep_pack) const noexcept;
		size_t prepare_data(psl::array_view<entity> entities, void* cache,
							details::component_key_t id) const noexcept;

		void prepare_system(std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
							std::uintptr_t cache_offset, details::system_information& information);

		psl::array_view<entity> entities_for(details::component_key_t key) const noexcept;
		psl::array_view<entity> entities_added_for(details::component_key_t key) const noexcept;
		psl::array_view<entity> entities_removed_for(details::component_key_t key) const noexcept;

		template <typename T>
		void create_storage()
		{
			auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [](const auto& cInfo) {
				constexpr auto key = details::key_for<T>();
				return (key == cInfo->id());
			});

			constexpr auto key = details::key_for<T>();
			if(it == std::end(m_Components)) m_Components.emplace_back(new details::component_info_typed<T>());
		}

		details::component_info* get_component_info(details::component_key_t key) noexcept;
		const details::component_info* get_component_info(details::component_key_t key) const noexcept;

		psl::array<const details::component_info*>
		get_component_info(psl::array_view<details::component_key_t> keys) const noexcept;
		template <typename T>
		details::component_info_typed<T>* get_component_typed_info()
		{
			auto it = std::find_if(std::begin(m_Components), std::end(m_Components), [](const auto& cInfo) {
				constexpr auto key{details::key_for<T>()};
				return cInfo->id() == key;
			});
			return ((details::component_info_typed<T>*)(it->operator->()));
		}
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
				create_storage<T>();
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &prototype);
			}
			else // todo wait till deduction guides are resolved on libc++
				 // https://bugs.llvm.org/show_bug.cgi?id=39606
			// else if constexpr(std::is_constructible<decltype(std::function(prototype)), T>::value)
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

				create_storage<type>();
				add_component_impl(details::key_for<type>(), entities, sizeof(type),
								   [prototype](std::uintptr_t location, size_t count) {
									   for(auto i = size_t{0}; i < count; ++i)
									   {
										   std::invoke(prototype, *((type*)(location) + i));
									   }
								   });
			}
			/*else
			{
				static_assert(psl::templates::always_false<T>::value,
							  "could not figure out if the template type was an invocable or a component prototype");
			}*/
		}

		template <typename T>
		void add_component(psl::array_view<std::pair<entity, entity>> entities, psl::ecs::empty<T>&& prototype)
		{
			create_storage<T>();
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
			create_storage<T>();
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
		void add_component(psl::array_view<entity> entities, T&& prototype)
		{

			if constexpr(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value &&
						 std::is_trivially_destructible<T>::value)
			{
				static_assert(!std::is_empty_v<T>,
							  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
							  "psl::ecs::empty<T>{} to avoid initialization.");


				create_storage<T>();
				add_component_impl(details::key_for<T>(), entities, sizeof(T), &prototype);
			}
			else
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
				create_storage<type>();
				add_component_impl(details::key_for<type>(), entities, sizeof(type),
								   [prototype](std::uintptr_t location, size_t count) {
									   for(auto i = size_t{0}; i < count; ++i)
									   {
										   std::invoke(prototype, *((type*)(location) + i));
									   }
								   });
			}
			/*else
			{
				static_assert(psl::templates::always_false<T>::value,
							  "could not figure out if the template type was an invocable or a component prototype");
			}*/
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities, psl::ecs::empty<T>&& prototype)
		{
			create_storage<T>();

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
		void add_component(psl::array_view<entity> entities)
		{
			create_storage<T>();
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

		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
								size_t size);
		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
								size_t size, std::function<void(std::uintptr_t, size_t)> invocable);
		void add_component_impl(details::component_key_t key, psl::array_view<std::pair<entity, entity>> entities,
								size_t size, void* prototype);


		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size);
		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
								std::function<void(std::uintptr_t, size_t)> invocable);
		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities, size_t size,
								void* prototype);

		//------------------------------------------------------------
		// remove_component
		//------------------------------------------------------------
		void remove_component(details::component_key_t key,
							  psl::array_view<std::pair<entity, entity>> entities) noexcept;
		void remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept;


		//------------------------------------------------------------
		// filter
		//------------------------------------------------------------
		template <typename T>
		psl::array<entity>::iterator filter_remove(psl::templates::proxy_type<T>, psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_remove(details::key_for<T>(), begin, end);
		}

		template <typename T>
		psl::array<entity>::iterator filter_remove(psl::templates::proxy_type<psl::ecs::filter<T>>,
												   psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_remove(details::key_for<T>(), begin, end);
		}
		template <typename T>
		psl::array<entity>::iterator filter_remove(psl::templates::proxy_type<psl::ecs::on_add<T>>,
												   psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_remove_on_add(details::key_for<T>(), begin, end);
		}
		template <typename T>
		psl::array<entity>::iterator filter_remove(psl::templates::proxy_type<psl::ecs::on_remove<T>>,
												   psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_remove_on_remove(details::key_for<T>(), begin, end);
		}
		template <typename T>
		psl::array<entity>::iterator filter_remove(psl::templates::proxy_type<psl::ecs::except<T>>,
												   psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_remove_except(details::key_for<T>(), begin, end);
		}
		template <typename... Ts>
		psl::array<entity>::iterator filter_remove(psl::templates::proxy_type<psl::ecs::on_break<Ts...>>,
												   psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_remove_on_break(to_keys<Ts...>(), begin, end);
		}

		template <typename... Ts>
		psl::array<entity>::iterator filter_remove(psl::templates::proxy_type<psl::ecs::on_combine<Ts...>>,
												   psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_remove_on_combine(to_keys<Ts...>(), begin, end);
		}

		psl::array<entity>::iterator filter_remove(details::component_key_t key, psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator filter_remove_on_add(details::component_key_t key,
														  psl::array<entity>::iterator& begin,
														  psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator filter_remove_on_remove(details::component_key_t key,
															 psl::array<entity>::iterator& begin,
															 psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator filter_remove_except(details::component_key_t key,
														  psl::array<entity>::iterator& begin,
														  psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator filter_remove_on_break(psl::array<details::component_key_t> keys,
															psl::array<entity>::iterator& begin,
															psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator filter_remove_on_combine(psl::array<details::component_key_t> keys,
															  psl::array<entity>::iterator& begin,
															  psl::array<entity>::iterator& end) const noexcept;

		template <typename T>
		psl::array<entity> filter_seed(psl::templates::proxy_type<T>) const noexcept
		{
			return filter_seed(details::key_for<T>());
		}

		template <typename T>
		psl::array<entity> filter_seed(psl::templates::proxy_type<psl::ecs::filter<T>>) const noexcept
		{
			return filter_seed(details::key_for<T>());
		}
		template <typename T>
		psl::array<entity> filter_seed(psl::templates::proxy_type<psl::ecs::on_add<T>>) const noexcept
		{
			return filter_seed_on_add(details::key_for<T>());
		}
		template <typename T>
		psl::array<entity> filter_seed(psl::templates::proxy_type<psl::ecs::on_remove<T>>) const noexcept
		{
			return filter_seed_on_remove(details::key_for<T>());
		}
		template <typename T>
		psl::array<entity> filter_seed(psl::templates::proxy_type<psl::ecs::except<T>>) const noexcept
		{
			static_assert(psl::templates::always_false_v<T>,
						  "you cannot start with a negative selector, this is unsupported for now");
		}
		template <typename... Ts>
		psl::array<entity> filter_seed(psl::templates::proxy_type<psl::ecs::on_break<Ts...>>) const noexcept
		{
			return filter_seed_on_break(to_keys<Ts...>());
		}

		template <typename... Ts>
		psl::array<entity> filter_seed(psl::templates::proxy_type<psl::ecs::on_combine<Ts...>>) const noexcept
		{
			return filter_seed_on_combine(to_keys<Ts...>());
		}

		psl::array<entity> filter_seed(details::component_key_t key) const noexcept;
		psl::array<entity> filter_seed_on_add(details::component_key_t key) const noexcept;
		psl::array<entity> filter_seed_on_remove(details::component_key_t key) const noexcept;
		psl::array<entity> filter_seed_on_break(psl::array<details::component_key_t> keys) const noexcept;
		psl::array<entity> filter_seed_on_combine(psl::array<details::component_key_t> keys) const noexcept;


		bool filter_seed_best(psl::array_view<details::component_key_t> filters, psl::array_view<details::component_key_t> added, psl::array_view<details::component_key_t> removed, psl::array_view<entity>& out, details::component_key_t& selected) const noexcept;
		bool filter_seed_best(const details::dependency_pack& pack, psl::array_view<entity>& out, details::component_key_t& selected) const noexcept;

		psl::array<entity> filter(details::dependency_pack& packs);
		template <typename... Ts>
		constexpr psl::array<details::component_key_t> to_keys() const noexcept
		{
			return psl::array<details::component_key_t>{details::key_for<Ts>()...};
		}

		//------------------------------------------------------------
		// set
		//------------------------------------------------------------
		void set(psl::array_view<entity> entities, details::component_key_t key, void* data) noexcept;


		//------------------------------------------------------------
		// system declare
		//------------------------------------------------------------
		template <typename... Ts>
		struct get_packs
		{
			using type = std::tuple<Ts...>;
		};

		template <typename... Ts>
		struct get_packs<std::tuple<Ts...>> : public get_packs<Ts...>
		{};

		template <typename... Ts>
		struct get_packs<psl::ecs::info&, Ts...> : public get_packs<Ts...>
		{};

		template <typename Fn, typename T = void>
		void declare_impl(threading threading, Fn&& fn, T* ptr = nullptr)
		{

			using function_args = typename psl::templates::func_traits<typename std::decay<Fn>::type>::arguments_t;
			std::function<std::vector<details::dependency_pack>()> pack_generator = []() {
				using pack_t = typename get_packs<function_args>::type;
				return details::expand_to_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
														  psl::templates::type_container<pack_t>{});
			};

			std::function<void(psl::ecs::info&, std::vector<details::dependency_pack>)> system_tick;

			if constexpr(std::is_member_function_pointer<Fn>::value)
			{
				system_tick = [fn, ptr](psl::ecs::info& info, std::vector<details::dependency_pack> packs) -> void {
					using pack_t = typename get_packs<function_args>::type;

					auto tuple_argument_list = std::tuple_cat(
						std::tuple<T*, psl::ecs::info&>(ptr, info),
						details::compress_from_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
															   psl::templates::type_container<pack_t>{}, packs));

					std::apply(fn, std::move(tuple_argument_list));
				};
			}
			else
			{
				system_tick = [fn](psl::ecs::info& info, std::vector<details::dependency_pack> packs) -> void {
					using pack_t = typename get_packs<function_args>::type;

					auto tuple_argument_list = std::tuple_cat(
						std::tuple<psl::ecs::info&>(info),
						details::compress_from_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
															   psl::templates::type_container<pack_t>{}, packs));

					std::apply(fn, std::move(tuple_argument_list));
				};
			}
			m_SystemInformations.emplace_back(threading, std::move(pack_generator), std::move(system_tick));
		}

		::memory::raw_region m_Cache{1024 * 1024 * 32};
		psl::array<entity> m_Entities;
		psl::array<psl::unique_ptr<details::component_info>> m_Components;
		std::vector<details::system_information> m_SystemInformations;

		psl::unique_ptr<psl::async::scheduler> m_Scheduler;

		entity m_Next;
		size_t m_Orphans{0};
		size_t m_Tick{0};
	};
} // namespace psl::ecs