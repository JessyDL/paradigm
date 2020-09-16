
#pragma once
#include "entity.h"
#include "selectors.h"
#include "psl/pack_view.h"
#include "command_buffer.h"
#include "psl/IDGenerator.h"
#include "psl/array.h"
#include "details/component_key.h"
#include "details/entity_info.h"
#include "details/component_info.h"
#include "details/system_information.h"
#include "psl/unique_ptr.h"
#include "psl/static_array.h"
#include <chrono>
#include "psl/async/async.hpp"
#include "psl/template_utils.h"
#include <functional>
#include "psl/memory/raw_region.h"
#include "entity.h"
#include "filtering.h"
#if __has_include(<execution>)
#include <execution>
#else
namespace std::execution
{
	class sequenced_policy
	{};

	inline constexpr sequenced_policy seq{};

	class parallel_policy
	{};

	inline constexpr parallel_policy par{};

	class parallel_unsequenced_policy
	{};

	inline constexpr parallel_unsequenced_policy par_unseq{};
} // namespace std::execution
#endif

namespace psl::async
{
	class scheduler;
}

namespace psl::ecs
{
	class state final
	{

		struct transform_result
		{
			bool operator==(const transform_result& other) const noexcept { return group == other.group; }
			psl::array<entity> entities;
			psl::array<entity> indices; // used in case there is an order_by
			std::shared_ptr<details::transform_group> group;
		};

		struct filter_result
		{
			bool operator==(const filter_result& other) const noexcept { return group == other.group; }
			bool operator==(const details::filter_group& other) const noexcept { return *group == other; }
			psl::array<entity> entities;
			std::shared_ptr<details::filter_group> group;

			// all transformations that will depend on this result
			psl::array<transform_result> transformations;
		};


	  public:
		state(size_t workers = 0, size_t cache_size = 1024 * 1024 * 256);
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
		void add_components(psl::array_view<entity> entities, psl::array_view<Ts>... data)
		{
			(add_component<Ts>(entities, data), ...);
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
		psl::ecs::pack<Ts...> get_components(psl::array_view<entity> entities) const noexcept;

		template <typename Pred, typename T>
		void order_by(psl::array<entity>::iterator begin, psl::array<entity>::iterator end) const noexcept
		{
			order_by<Pred, T>(std::execution::seq, begin, end);
		}
		template <typename Pred, typename T>
		void order_by(std::execution::sequenced_policy, psl::array<entity>::iterator begin,
					  psl::array<entity>::iterator end) const noexcept
		{
			const auto pred = Pred{};
			std::sort(std::execution::seq, begin, end, [this, &pred](entity lhs, entity rhs) -> bool {
				return std::invoke(pred, this->get<T>(lhs), this->get<T>(rhs));
			});
		}


	  private:
		template <typename Pred, typename T>
		void order_by(std::execution::parallel_policy, psl::array<entity>::iterator begin,
					  psl::array<entity>::iterator end, size_t max) const noexcept
		{
			auto size = std::distance(begin, end);
			if(size <= static_cast<decltype(size)>(max))
			{
				order_by<Pred, T>(std::execution::seq, begin, end);
			}
			else
			{
				auto middle = std::next(begin, size / 2);
				auto future = std::async(
					[this, &begin, &middle, &max]() { order_by<Pred, T>(std::execution::par, begin, middle, max); });

				order_by<Pred, T>(std::execution::par, middle, end, max);

				const auto pred = Pred{};
				future.wait();
				std::inplace_merge(std::execution::par_unseq, begin, middle, end,
								   [this, &pred](entity lhs, entity rhs) -> bool {
									   return std::invoke(pred, this->get<T>(lhs), this->get<T>(rhs));
								   });
			}
		}

	  public:
		void clear() noexcept;
		template <typename Pred, typename T>
		void order_by(std::execution::parallel_policy, psl::array<entity>::iterator begin,
					  psl::array<entity>::iterator end) const noexcept
		{
			auto size = std::distance(begin, end);
			auto thread_size =
				std::max<size_t>(1u, std::min<size_t>(std::thread::hardware_concurrency(), size % 1024u));
			size /= thread_size;


			order_by<Pred, T>(std::execution::par, begin, end, size);
		}


		template <typename Pred, typename T>
		psl::array<entity>::iterator on_condition(psl::array<entity>::iterator begin,
												  psl::array<entity>::iterator end) const noexcept
		{
			auto pred = Pred{};
			return std::remove_if(std::execution::par_unseq, begin, end,
								  [this, &pred](entity lhs) -> bool { return pred(this->get<T>(lhs)); });
		}

		template <typename T, typename Pred>
		psl::array<entity>::iterator on_condition(psl::array<entity>::iterator begin, psl::array<entity>::iterator end,
												  Pred&& pred) const noexcept
		{
			return std::remove_if(std::execution::par_unseq, begin, end,
								  [this, &pred](entity lhs) -> bool { return pred(this->get<T>(lhs)); });
		}

		template <typename T>
		T& get(entity entity)
		{
			// todo this should support filtering
			auto cInfo = get_component_typed_info<T>();
			return cInfo->entity_data().at(entity, 0, 2);
		}

		template <typename T>
		const T& get(entity entity) const noexcept
		{
			// todo this should support filtering
			auto cInfo = get_component_typed_info<T>();
			return cInfo->entity_data().at(entity, 0, 2);
		}

		template <typename... Ts>
		bool has_components(psl::array_view<entity> entities) const noexcept
		{
			auto cInfos = get_component_info(to_keys<Ts...>());
			if(cInfos.size() == sizeof...(Ts))
			{
				return std::all_of(std::begin(cInfos), std::end(cInfos), [&entities](const auto& cInfo) {
					return cInfo && std::all_of(std::begin(entities), std::end(entities),
												[&cInfo](entity e) { return cInfo->has_component(e); });
				});
			}
			return sizeof...(Ts) == 0;
		}

		[[maybe_unused]] entity create()
		{
			if(m_Orphans.size() > 0)
			{
				auto entity = m_Orphans.back();
				m_Orphans.pop_back();
				return entity;
			}
			else
			{
				return m_Entities++;
			}
		}

		template <typename... Ts>
		[[maybe_unused]] psl::array<entity> create(entity count)
		{
			psl::array<entity> entities;
			entities.reserve(count);
			const auto recycled	 = std::min<entity>(count, (entity)m_Orphans.size());
			const auto remainder = count - recycled;

			std::reverse_copy(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans),
							  std::back_inserter(entities));
			m_Orphans.erase(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans));
			entities.resize(count);
			std::iota(std::next(std::begin(entities), recycled), std::end(entities), m_Entities);
			m_Entities += remainder;

			if constexpr(sizeof...(Ts) > 0)
			{
				(add_components<Ts>(entities), ...);
			}
			return entities;
		}

		template <typename... Ts>
		[[maybe_unused]] psl::array<entity> create(entity count, Ts&&... prototype)
		{
			psl::array<entity> entities;
			entities.reserve(count);
			const auto recycled	 = std::min<entity>(count, static_cast<entity>(m_Orphans.size()));
			const auto remainder = count - recycled;

			std::reverse_copy(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans),
							  std::back_inserter(entities));
			m_Orphans.erase(std::prev(std::end(m_Orphans), recycled), std::end(m_Orphans));
			entities.resize(count);
			std::iota(std::next(std::begin(entities), recycled), std::end(entities), m_Entities);
			m_Entities += remainder;

			add_components(entities, std::forward<Ts>(prototype)...);

			return entities;
		}

		void destroy(psl::array_view<entity> entities) noexcept;
		void destroy(entity entity) noexcept;

		psl::array<entity> all_entities() const noexcept
		{
			auto orphans = m_Orphans;
			std::sort(std::begin(orphans), std::end(orphans));

			auto orphan_it = std::begin(orphans);
			psl::array<entity> result;
			result.reserve(m_Entities - orphans.size());
			for(entity e = 0; e < m_Entities; ++e)
			{
				if(orphan_it != std::end(m_Orphans) && e == *orphan_it)
				{
					orphan_it = std::next(orphan_it);
					continue;
				}
				result.emplace_back(e);
			}
			return result;
		}

		template <typename... Ts>
		psl::array<entity> filter() const noexcept
		{
			auto filter_group = details::make_filter_group(psl::templates::type_container<psl::ecs::pack<Ts...>>{});

			auto it = std::find_if(std::begin(m_Filters), std::end(m_Filters),
								   [&filter_group](const filter_result& data) { return *data.group == filter_group; });

			if(it != std::end(m_Filters))
			{
				auto modified = psl::array<entity>{m_ModifiedEntities.indices()};
				std::sort(std::begin(modified), std::end(modified));

				filter_result data{{}, it->group};
				// data = *it;
				filter(data, modified);
				return data.entities;
			}
			else
			{
				filter_result data{{}, std::make_shared<details::filter_group>(filter_group)};
				filter(data);
				return data.entities;
			}
		}
		template <typename... Ts>
		void set_components(psl::array_view<entity> entities, psl::array_view<Ts>... data) noexcept
		{
			(set_component(entities, std::forward<Ts>(data)), ...);
		}

		template <typename... Ts>
		void set_components(psl::array_view<entity> entities, Ts&&... data) noexcept
		{
			(set_component(entities, std::forward<Ts>(data)), ...);
		}

		template <typename T>
		void set_component(psl::array_view<entity> entities, T&& data) noexcept
		{
			constexpr auto key = details::key_for<T>();
			auto cInfo		   = get_component_info(key);
			for(auto e : entities)
			{
				cInfo->set(e, &data);
			}
		}

		template <typename T>
		void set_component(psl::array_view<entity> entities, psl::array_view<T> data) noexcept
		{
			assert(entities.size() == data.size());
			constexpr auto key = details::key_for<T>();
			auto cInfo		   = get_component_info(key);
			auto d			   = std::begin(data);
			for(auto e : entities)
			{
				cInfo->set(e, *d);
				d = std::next(d);
			}
		}

		void tick(std::chrono::duration<float> dTime);

		void reset(psl::array_view<entity> entities) noexcept;

		template <typename T>
		psl::array_view<entity> entities() const noexcept
		{
			constexpr auto key{details::key_for<T>()};
			if(auto it = m_Components.find(key); it != std::end(m_Components)) return it->second->entities();
			return {};
		}

		template <typename T>
		psl::array_view<T> view()
		{
			constexpr auto key{details::key_for<T>()};
			if(auto it = m_Components.find(key); it != std::end(m_Components))
				return ((details::component_info_typed<T>*)(&it->second.get()))->entity_data().dense(0, 1);
			return {};
		}

		/// \brief returns the amount of active systems
		size_t systems() const noexcept { return m_SystemInformations.size() - m_ToRevoke.size(); }

		template <typename Fn>
		auto declare(Fn&& fn, bool seedWithExisting = false)
		{
			return declare_impl(threading::sequential, std::forward<Fn>(fn), (void*)nullptr, seedWithExisting);
		}


		template <typename Fn>
		auto declare(threading threading, Fn&& fn, bool seedWithExisting = false)
		{
			return declare_impl(threading, std::forward<Fn>(fn), (void*)nullptr, seedWithExisting);
		}

		template <typename Fn, typename T>
		auto declare(Fn&& fn, T* ptr, bool seedWithExisting = false)
		{
			return declare_impl(threading::sequential, std::forward<Fn>(fn), ptr, seedWithExisting);
		}
		template <typename Fn, typename T>
		auto declare(threading threading, Fn&& fn, T* ptr, bool seedWithExisting = false)
		{
			return declare_impl(threading, std::forward<Fn>(fn), ptr, seedWithExisting);
		}

		bool revoke(details::system_token id) noexcept
		{
			if(m_LockState)
			{
				if(auto it = std::find_if(std::begin(m_SystemInformations), std::end(m_SystemInformations),
										  [&id](const auto& system) { return system.id() == id; });
				   it != std::end(m_SystemInformations) &&
				   // and we make sure we don't "double delete"
				   std::find(std::begin(m_ToRevoke), std::end(m_ToRevoke), id) == std::end(m_ToRevoke))
				{
					m_ToRevoke.emplace_back(id);
					return true;
				}
				return false;
			}
			else
			{
				if(auto it = std::find_if(std::begin(m_SystemInformations), std::end(m_SystemInformations),
										  [&id](const auto& system) { return system.id() == id; });
				   it != std::end(m_SystemInformations))
				{
					m_SystemInformations.erase(it);
					return true;
				}
				return false;
			}
		}

		size_t capacity() const noexcept { return m_Entities; }

		template <typename... Ts>
		size_t size() const noexcept
		{
			// todo implement filtering?
			if constexpr(sizeof...(Ts) == 0)
			{
				return m_Entities - m_Orphans.size();
			}
			else
			{
				return size(to_keys<Ts...>());
			}
		}

		size_t size(psl::array_view<details::component_key_t> keys) const noexcept;

	  private:
		//------------------------------------------------------------
		// helpers
		//------------------------------------------------------------
		size_t prepare_bindings(psl::array_view<entity> entities, void* cache, details::dependency_pack& dep_pack) const
			noexcept;
		size_t prepare_data(psl::array_view<entity> entities, void* cache, details::component_key_t id) const noexcept;

		void prepare_system(std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
							std::uintptr_t cache_offset, details::system_information& information);


		void execute_command_buffer(info& info);

		template <typename T>
		inline void create_storage() const noexcept
		{
			constexpr auto key = details::key_for<T>();
			if(auto it = m_Components.find(key); it == std::end(m_Components))
				m_Components.emplace(key, new details::component_info_typed<T>());
		}

		details::component_info* get_component_info(details::component_key_t key) noexcept;
		const details::component_info* get_component_info(details::component_key_t key) const noexcept;

		psl::array<const details::component_info*>
		get_component_info(psl::array_view<details::component_key_t> keys) const noexcept;
		template <typename T>
		details::component_info_typed<T>* get_component_typed_info() const noexcept
		{
			constexpr auto key{details::key_for<T>()};
			return (details::component_info_typed<T>*)&m_Components.at(key).get();
		}
		//------------------------------------------------------------
		// add_component
		//------------------------------------------------------------

		template <typename T>
		void add_component(psl::array_view<entity> entities, T&& prototype)
		{
			using true_type = std::remove_const_t<std::remove_reference_t<T>>;
			if constexpr(psl::ecs::details::is_empty_container<true_type>::value)
			{
				using type = typename psl::ecs::details::empty_container<true_type>::type;
				create_storage<type>();
				if constexpr(std::is_trivially_constructible_v<type>)
				{
					add_component_impl(details::key_for<type>(), entities);
				}
				else
				{
					type v{};
					add_component_impl(details::key_for<type>(), entities, &v);
				}
			}
			else if constexpr(psl::templates::is_callable_n<T, 1>::value)
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
				add_component_impl(details::key_for<type>(), entities,
								   [prototype](std::uintptr_t location, size_t count) {
									   for(auto i = size_t{0}; i < count; ++i)
									   {
										   std::invoke(prototype, *((type*)(location) + i));
									   }
								   });
			}
			else if constexpr(/*std::is_trivially_copyable<T>::value && */std::is_standard_layout<T>::value/* &&
							  std::is_trivially_destructible<T>::value*/)
			{
				static_assert(!std::is_empty_v<T>,
							  "Unnecessary initialization of component tag, you likely didn't mean this. Wrap tags in "
							  "psl::ecs::empty<T>{} to avoid initialization.");


				create_storage<T>();
				add_component_impl(details::key_for<T>(), entities, &prototype);
			}
			else if constexpr (details::is_range_t<true_type>::value)
			{
				using type = typename psl::ecs::details::is_range_t<true_type>::type;
				create_storage<type>();
				add_component_impl(details::key_for<type>(), entities, prototype.data(), false);
			}
			else
			{
				static_assert(psl::templates::always_false<T>::value,
							  "could not figure out if the template type was an invocable or a component prototype");
			}
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities)
		{
			create_storage<T>();
			if constexpr(std::is_trivially_constructible_v<T>)
			{
				add_component_impl(details::key_for<T>(), entities);
			}
			else
			{
				T v{};
				add_component_impl(details::key_for<T>(), entities, &v);
			}
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities, psl::array_view<T> data)
		{
			assert(entities.size() == data.size());
			create_storage<T>();
			static_assert(!std::is_empty_v<T>,
						  "no need to pass an array of tag types through, it's a waste of computing and memory");

			add_component_impl(details::key_for<T>(), entities, data.data(), false);
		}


		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities);
		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities,
								std::function<void(std::uintptr_t, size_t)> invocable);
		void add_component_impl(details::component_key_t key, psl::array_view<entity> entities, void* prototype,
								bool repeat = true);

		//------------------------------------------------------------
		// remove_component
		//------------------------------------------------------------
		void remove_component(details::component_key_t key, psl::array_view<entity> entities) noexcept;


		//------------------------------------------------------------
		// filter
		//------------------------------------------------------------
		template <typename T>
		psl::array<entity>::iterator filter_op(psl::templates::proxy_type<T>, psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_op(details::key_for<T>(), begin, end);
		}

		template <typename T>
		psl::array<entity>::iterator filter_op(psl::templates::proxy_type<psl::ecs::filter<T>>,
											   psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept
		{
			return filter_op(details::key_for<T>(), begin, end);
		}
		template <typename T>
		psl::array<entity>::iterator filter_op(psl::templates::proxy_type<psl::ecs::on_add<T>>,
											   psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept
		{
			return on_add_op(details::key_for<T>(), begin, end);
		}
		template <typename T>
		psl::array<entity>::iterator filter_op(psl::templates::proxy_type<psl::ecs::on_remove<T>>,
											   psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept
		{
			return on_remove_op(details::key_for<T>(), begin, end);
		}
		template <typename T>
		psl::array<entity>::iterator filter_op(psl::templates::proxy_type<psl::ecs::except<T>>,
											   psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept
		{
			return on_except_op(details::key_for<T>(), begin, end);
		}
		template <typename... Ts>
		psl::array<entity>::iterator filter_op(psl::templates::proxy_type<psl::ecs::on_break<Ts...>>,
											   psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept
		{
			return on_break_op(to_keys<Ts...>(), begin, end);
		}

		template <typename... Ts>
		psl::array<entity>::iterator filter_op(psl::templates::proxy_type<psl::ecs::on_combine<Ts...>>,
											   psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept
		{
			return on_combine_op(to_keys<Ts...>(), begin, end);
		}

		psl::array<entity>::iterator filter_op(details::component_key_t key, psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator on_add_op(details::component_key_t key, psl::array<entity>::iterator& begin,
											   psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator on_remove_op(details::component_key_t key, psl::array<entity>::iterator& begin,
												  psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator on_except_op(details::component_key_t key, psl::array<entity>::iterator& begin,
												  psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator on_break_op(psl::array<details::component_key_t> keys,
												 psl::array<entity>::iterator& begin,
												 psl::array<entity>::iterator& end) const noexcept;
		psl::array<entity>::iterator on_combine_op(psl::array<details::component_key_t> keys,
												   psl::array<entity>::iterator& begin,
												   psl::array<entity>::iterator& end) const noexcept;

		psl::array<entity> filter(details::dependency_pack& pack, bool seed_with_previous) const noexcept;
		void filter(filter_result& data, psl::array_view<entity> source) const noexcept;
		void filter(filter_result& data, bool seed_with_previous = false) const noexcept;


		//------------------------------------------------------------
		// transformations
		//------------------------------------------------------------

		void transform(transform_result& data, psl::array_view<entity> source) const noexcept;
		void transform(transform_result& data) const noexcept;


		template <typename... Ts>
		psl::array<details::component_key_t> to_keys() const noexcept
		{
			return psl::array<details::component_key_t>{details::key_for<Ts>()...};
		}


		//------------------------------------------------------------
		// set
		//------------------------------------------------------------
		size_t set(psl::array_view<entity> entities, details::component_key_t key, void* data) noexcept;


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

		std::pair<std::shared_ptr<details::filter_group>, std::shared_ptr<details::transform_group>>
		add_filter_group(details::filter_group& filter_group, details::transform_group& transform_group)
		{
			std::shared_ptr<details::transform_group> shared_transform_group{};
			auto filter_it =
				std::find_if(std::begin(m_Filters), std::end(m_Filters),
							 [&filter_group](const filter_result& data) { return *data.group == filter_group; });
			if(filter_it == std::end(m_Filters))
			{
				m_Filters.emplace_back(filter_result{{}, std::make_shared<details::filter_group>(filter_group)});
				filter_it = std::prev(std::end(m_Filters));
			}

			if(transform_group)
			{
				auto it = std::find_if(
					std::begin(filter_it->transformations), std::end(filter_it->transformations),
					[&transform_group](const transform_result& data) { return *data.group == transform_group; });
				if(it == std::end(filter_it->transformations))
				{
					filter_it->transformations
						.emplace_back(
							transform_result{{}, {}, std::make_shared<details::transform_group>(transform_group)})
						.group;

					it = std::prev(std::end(filter_it->transformations));
				}

				return {filter_it->group, it->group};
			}

			return {filter_it->group, {}};
		}

		template <typename Fn, typename T = void>
		auto declare_impl(threading threading, Fn&& fn, T* ptr, bool seedWithExisting = false)
		{

			using function_args	  = typename psl::templates::func_traits<typename std::decay<Fn>::type>::arguments_t;
			using pack_t		  = typename get_packs<function_args>::type;
			auto filter_groups	  = details::make_filter_group(psl::templates::type_container<pack_t>{});
			auto transform_groups = details::make_transform_group(psl::templates::type_container<pack_t>{});
			std::function<psl::array<details::dependency_pack>(bool)> pack_generator = [](bool seedWithPrevious =
																							  false) {
				return details::expand_to_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
														  psl::templates::type_container<pack_t>{}, seedWithPrevious);
			};

			std::function<void(psl::ecs::info&, psl::array<details::dependency_pack>)> system_tick;

			if constexpr(std::is_member_function_pointer<Fn>::value)
			{
				system_tick = [fn, ptr](psl::ecs::info& info, psl::array<details::dependency_pack> packs) -> void {
					auto tuple_argument_list = std::tuple_cat(
						std::tuple<T*, psl::ecs::info&>(ptr, info),
						details::compress_from_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
															   psl::templates::type_container<pack_t>{}, packs));

					std::apply(fn, std::move(tuple_argument_list));
				};
			}
			else
			{
				system_tick = [fn](psl::ecs::info& info, psl::array<details::dependency_pack> packs) -> void {
					auto tuple_argument_list = std::tuple_cat(
						std::tuple<psl::ecs::info&>(info),
						details::compress_from_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
															   psl::templates::type_container<pack_t>{}, packs));

					std::apply(fn, std::move(tuple_argument_list));
				};
			}

			auto& sys_info = (m_LockState) ? m_NewSystemInformations : m_SystemInformations;

			if constexpr(std::is_same_v<details::filter_group, decltype(filter_groups)>)
			{
				auto [shared_filter, transform_filter] = add_filter_group(filter_groups, transform_groups);

				sys_info.emplace_back(threading, std::move(pack_generator), std::move(system_tick),
									  psl::array<std::shared_ptr<details::filter_group>>{shared_filter},
									  psl::array<std::shared_ptr<details::transform_group>>{transform_filter},
									  ++m_SystemCounter, seedWithExisting);
				return sys_info[sys_info.size() - 1].id();
			}
			else
			{
				psl::array<std::shared_ptr<details::filter_group>> shared_filter_groups;
				psl::array<std::shared_ptr<details::transform_group>> shared_transform_groups;

				for(auto i = 0; i < filter_groups.size(); ++i)
				{
					auto [shared_filter, transform_filter] = add_filter_group(filter_groups[i], transform_groups[i]);
					shared_filter_groups.emplace_back(shared_filter);
					shared_transform_groups.emplace_back(transform_filter);
				}


				sys_info.emplace_back(threading, std::move(pack_generator), std::move(system_tick),
									  shared_filter_groups, shared_transform_groups, ++m_SystemCounter,
									  seedWithExisting);
				return sys_info[sys_info.size() - 1].id();
			}
		}

		::memory::raw_region m_Cache{1024 * 1024 * 256};
		psl::array<psl::unique_ptr<info>> info_buffer{};
		psl::array<entity> m_Orphans{};
		psl::array<entity> m_ToBeOrphans{};
		mutable psl::array<filter_result> m_Filters{};
		psl::array<details::system_information> m_SystemInformations{};

		psl::array<details::system_token> m_ToRevoke{};
		psl::array<details::system_information> m_NewSystemInformations{};

		mutable std::unordered_map<details::component_key_t, psl::unique_ptr<details::component_info>> m_Components{};

		psl::sparse_indice_array<entity> m_ModifiedEntities{};

		psl::unique_ptr<psl::async::scheduler> m_Scheduler{nullptr};

		size_t m_LockState{0};
		size_t m_Tick{0};
		size_t m_SystemCounter{0};
		entity m_Entities{0};
	};

	namespace details
	{
		template <typename Pred, typename... Ts>
		void dependency_pack::select_ordering_impl(std::pair<Pred, std::tuple<Ts...>>)
		{
			static_assert(sizeof...(Ts) == 1, "due to a bug in MSVC we cannot have deeper nested template packs");
			orderby = [](psl::array<entity>::iterator begin, psl::array<entity>::iterator end,
						 const psl::ecs::state& state) {
				state.order_by<Pred, Ts...>(std::execution::par, begin, end);
			};
		}

		template <typename Pred, typename... Ts>
		void dependency_pack::select_condition_impl(std::pair<Pred, std::tuple<Ts...>>)
		{
			on_condition.push_back([](psl::array<entity>::iterator begin, psl::array<entity>::iterator end,
									  const psl::ecs::state& state) -> psl::array<entity>::iterator {
				return state.on_condition<Pred, Ts...>(begin, end);
			});
		}

	} // namespace details
} // namespace psl::ecs