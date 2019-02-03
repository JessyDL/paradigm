#pragma once

#include "entity.h"
#include "range.h"
#include "selectors.h"
#include "pack.h"
#include "memory/raw_region.h"
#include "IDGenerator.h"
#include <type_traits>
#include "event.h"
#include "logging.h"
#include <numeric>
#include "enumerate.h"
#include "component_key.h"
#include "component_info.h"
#include "state_operations.h"
#include "template_utils.h"
#include "memory/region.h"
namespace psl::async
{
	class scheduler;
}

/// \brief Entity Component System
///
/// The ECS namespace contains a fully functioning ECS
namespace core::ecs
{
	enum class threading
	{
		seq		   = 0,
		sequential = seq,
		par		   = 1,
		parallel   = par,
		main	   = 2
	};

	struct tick
	{};
	struct pre_tick
	{};
	struct post_tick
	{};

	class state;
	class command_buffer;


	namespace details
	{
		using tick_t =
			std::function<void(core::ecs::command_buffer&, std::chrono::duration<float>, std::chrono::duration<float>)>;
		using pre_tick_t  = std::function<void(core::ecs::command_buffer&)>;
		using post_tick_t = std::function<void(core::ecs::command_buffer&)>;


		template <typename T>
		struct type_container
		{
			using type = T;
		};

		template <typename T, typename SFINEA = void>
		struct mf_pre_tick : std::false_type
		{};

		/// \brief SFINAE tag that is used to detect the method signature for the `pre_tick` listener.
		template <typename T>
		struct mf_pre_tick<
			T, std::void_t<decltype(std::declval<T&>().pre_tick(std::declval<core::ecs::command_buffer&>()))>>
			: std::true_type
		{};

		template <typename T, typename SFINEA = void>
		struct mf_post_tick : std::false_type
		{};

		/// \brief SFINAE tag that is used to detect the method signature for the `post_tick` listener.
		template <typename T>
		struct mf_post_tick<
			T, std::void_t<decltype(std::declval<T&>().post_tick(std::declval<core::ecs::command_buffer&>()))>>
			: std::true_type
		{};


		template <typename T, typename SFINEA = void>
		struct mf_tick : std::false_type
		{};

		/// \brief SFINAE tag that is used to detect the method signature for the `tick` listener.
		template <typename T>
		struct mf_tick<T, std::void_t<decltype(std::declval<T&>().tick(std::declval<core::ecs::command_buffer&>(),
																	   std::declval<std::chrono::duration<float>>(),
																	   std::declval<std::chrono::duration<float>>()))>>
			: std::true_type
		{};


		/// \brief describes a set of dependencies for a given system
		///
		/// systems can have various dependencies, for example a movement system could have
		/// dependencies on both a core::ecs::components::transform component and a core::ecs::components::renderable
		/// component. This dependency will output a set of core::ecs::entity's that have all required
		/// core::ecs::components present. Certain systems could have sets of dependencies, for example the render
		/// system requires knowing about both all `core::ecs::components::renderable` that have a
		/// `core::ecs::components::transform`, but also needs to know all `core::ecs::components::camera's`. So that
		/// system would require several dependency_pack's.
		class owner_dependency_pack
		{
			friend class core::ecs::state;
			template <std::size_t... Is, typename T>
			auto create_dependency_filters(std::index_sequence<Is...>, type_container<T>)
			{
				(add(type_container<
					 typename std::remove_reference<decltype(std::declval<T>().template get<Is>())>::type>{}),
				 ...);
			}

			template <typename F>
			void select_impl(std::vector<component_key_t>& target)
			{
				if constexpr(!std::is_same<details::remove_all<F>, core::ecs::entity>::value)
				{
					using component_t			  = F;
					constexpr component_key_t key = details::component_key<details::remove_all<component_t>>;
					target.emplace_back(key);
					m_Sizes[key] = sizeof(component_t);
				}
			}

			template <std::size_t... Is, typename T>
			auto select(std::index_sequence<Is...>, T, std::vector<component_key_t>& target)
			{
				(select_impl<typename std::tuple_element<Is, T>::type>(target), ...);
			}

			template <typename T>
			psl::array_view<T> fill_in(type_container<psl::array_view<T>>)
			{
				if constexpr(std::is_same<T, core::ecs::entity>::value)
				{
					return m_Entities;
				}
				else if constexpr(std::is_const<T>::value)
				{
					constexpr component_key_t int_id = details::component_key<details::remove_all<T>>;
					return *(psl::array_view<T>*)&m_RBindings[int_id];
				}
				else
				{
					constexpr component_key_t int_id = details::component_key<details::remove_all<T>>;
					return *(psl::array_view<T>*)&m_RWBindings[int_id];
				}
			}

			template <std::size_t... Is, typename T>
			T to_pack_impl(std::index_sequence<Is...>, type_container<T>)
			{
				using pack_t	  = T;
				using pack_view_t = typename pack_t::pack_t;
				using range_t	 = typename pack_t::pack_t::range_t;

				return T{pack_view_t(fill_in(type_container<typename std::tuple_element<Is, range_t>::type>())...)};
			}

		  public:
			template <typename T>
			owner_dependency_pack(type_container<T>)
			{
				using pack_t = T;
				create_dependency_filters(
					std::make_index_sequence<std::tuple_size_v<typename pack_t::pack_t::range_t>>{},
					type_container<T>{});
				select(std::make_index_sequence<std::tuple_size<typename pack_t::filter_t>::value>{},
					   typename pack_t::filter_t{}, filters);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::add_t>::value>{},
					   typename pack_t::add_t{}, on_add);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::remove_t>::value>{},
					   typename pack_t::remove_t{}, on_remove);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::break_t>::value>{},
					   typename pack_t::break_t{}, on_break);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::combine_t>::value>{},
					   typename pack_t::combine_t{}, on_combine);
				select(std::make_index_sequence<std::tuple_size<typename pack_t::except_t>::value>{},
					   typename pack_t::except_t{}, except);

				if constexpr(std::is_same<core::ecs::partial, typename pack_t::policy_t>::value)
				{
					m_IsPartial = true;
				}
			};


			~owner_dependency_pack() noexcept						  = default;
			owner_dependency_pack(const owner_dependency_pack& other) = default;
			owner_dependency_pack(owner_dependency_pack&& other)	  = default;
			owner_dependency_pack& operator=(const owner_dependency_pack&) = default;
			owner_dependency_pack& operator=(owner_dependency_pack&&) = default;


			template <typename... Ts>
			core::ecs::pack<Ts...> to_pack(type_container<core::ecs::pack<Ts...>>)
			{
				using pack_t  = core::ecs::pack<Ts...>;
				using range_t = typename pack_t::pack_t::range_t;

				return to_pack_impl(std::make_index_sequence<std::tuple_size<range_t>::value>{},
									type_container<pack_t>{});
			}

			bool allow_partial() const noexcept { return m_IsPartial; };

			size_t size_per_element() const noexcept
			{
				size_t res{0};
				for(const auto& binding : m_RBindings)
				{
					res += m_Sizes.at(binding.first);
				}

				for(const auto& binding : m_RWBindings)
				{
					res += m_Sizes.at(binding.first);
				}
				return res;
			}

			size_t entities() const noexcept
			{
				return m_Entities.size();
			}
			owner_dependency_pack slice(size_t begin, size_t end) const noexcept;
		  private:
			template <typename T>
			void add(type_container<psl::array_view<T>>) noexcept
			{
				constexpr component_key_t int_id = details::component_key<details::remove_all<T>>;
				m_RWBindings.emplace(int_id, psl::array_view<std::uintptr_t>{});
			}

			template <typename T>
			void add(type_container<psl::array_view<const T>>) noexcept
			{
				constexpr component_key_t int_id = details::component_key<details::remove_all<T>>;
				m_RBindings.emplace(int_id, psl::array_view<std::uintptr_t>{});
			}


			void add(type_container<psl::array_view<core::ecs::entity>>) noexcept {}
			void add(type_container<psl::array_view<const core::ecs::entity>>) noexcept {}

		  private:
			psl::array_view<core::ecs::entity> m_Entities{};
			details::key_value_container_t<component_key_t, size_t> m_Sizes;
			details::key_value_container_t<component_key_t, psl::array_view<std::uintptr_t>> m_RBindings;
			details::key_value_container_t<component_key_t, psl::array_view<std::uintptr_t>> m_RWBindings;

			std::vector<component_key_t> filters;
			std::vector<component_key_t> on_add;
			std::vector<component_key_t> on_remove;
			std::vector<component_key_t> except;
			std::vector<component_key_t> on_combine;
			std::vector<component_key_t> on_break;
			bool m_IsPartial = false;
		};

		template <std::size_t... Is, typename T>
		std::vector<owner_dependency_pack> expand_to_dependency_pack(std::index_sequence<Is...>, type_container<T>)
		{
			std::vector<owner_dependency_pack> res;
			(std::invoke([&]() {
				 res.emplace_back(owner_dependency_pack(type_container<typename std::tuple_element<Is, T>::type>{}));
			 }),
			 ...);
			return res;
		}


		template <std::size_t... Is, typename... Ts>
		std::tuple<Ts...> compress_from_dependency_pack(std::index_sequence<Is...>, type_container<std::tuple<Ts...>>,
														std::vector<owner_dependency_pack>& pack)
		{
			return std::tuple<Ts...>{
				pack[Is].to_pack(type_container<typename std::remove_reference<decltype(
									 std::get<Is>(std::declval<std::tuple<Ts...>>()))>::type>{})...};
		}

		struct system_information
		{
			threading threading = threading::sequential;
			std::function<std::vector<details::owner_dependency_pack>()> pack_generator;
			std::function<core::ecs::command_buffer(const core::ecs::state&, std::chrono::duration<float>,
													std::chrono::duration<float>,
													std::vector<details::owner_dependency_pack>)>
				invocable;
		};
	} // namespace details
} // namespace core::ecs

namespace std
{
	template <>
	struct hash<core::ecs::details::component_info>
	{
		std::size_t operator()(core::ecs::details::component_info const& ci) const noexcept
		{
			static_assert(sizeof(size_t) == sizeof(core::ecs::component_key_t), "should be castable");
			return (size_t)ci.id;
		}
	};
} // namespace std

namespace core::ecs
{
	class state final
	{
		friend class core::ecs::command_buffer;

	  public:
		state();
		~state();

	  private:
		

		template <typename Fn>
		void copy_components(component_key_t key, psl::array_view<entity> entities, const size_t component_size,
							 Fn&& function)
		{
			static_assert(std::is_same<typename std::invoke_result<Fn, entity>::type, std::uintptr_t>::value,
						  "the return value must be a location in memory");

			if(entities.size() == 0) return;

			auto& cInfo{details::get_component_info(key, component_size, m_Components)};

			uint64_t component_location{0};
			if(component_size == 0)
			{
				std::vector<entity> merged_set;
				std::set_union(std::begin(cInfo.entities), std::end(cInfo.entities), std::begin(entities),
							   std::end(entities), std::back_inserter(merged_set));
				cInfo.entities = std::move(merged_set);
				for(auto e : entities)
				{
					m_EntityMap[e].emplace_back(std::pair<component_key_t, size_t>{key, component_location});
				}
			}
			else if((entities.size() == 1 && cInfo.generator.try_create(component_location)) ||
					cInfo.generator.try_create(component_location, entities.size()))
			{
				std::vector<entity> merged_set;
				std::set_union(std::begin(cInfo.entities), std::end(cInfo.entities), std::begin(entities),
							   std::end(entities), std::back_inserter(merged_set));
				cInfo.entities = std::move(merged_set);
				for(auto e : entities)
				{
					std::uintptr_t source = std::invoke(function, e);
					std::uintptr_t destination =
						(std::uintptr_t)cInfo.region.data() + component_location * component_size;
					std::memcpy((void*)destination, (void*)source, component_size);
					m_EntityMap[e].emplace_back(std::pair<component_key_t, size_t>{key, component_location});
					++component_location;
				}
			}
			else
			{
				auto batch_size = entities.size() / 2;
				if(batch_size > 2)
				{
					copy_components(key, psl::array_view<entity>(std::begin(entities), batch_size), component_size,
									std::forward<Fn>(function));
					copy_components(key,
									psl::array_view<entity>(std::next(std::begin(entities), batch_size),
															entities.size() - batch_size),
									component_size, std::forward<Fn>(function));
				}
				else
				{
					for(auto it = std::begin(entities); it != std::end(entities); it = std::next(it))
					{
						copy_components(key, psl::array_view<entity>(it, 1), component_size,
										std::forward<Fn>(function));
					}
				}
			}
		}
		void destroy_component_generator_ids(details::component_info& cInfo, psl::array_view<entity> entities);

	  private:
		// -----------------------------------------------------------------------------
		// add component
		// -----------------------------------------------------------------------------

		template <typename T>
		void add_component(psl::array_view<entity> entities, T&& _template) noexcept
		{
			auto added_entities{
				details::add_component(m_EntityMap, m_Components, entities, std::forward<T>(_template))};

			using component_type		  = typename details::get_component_type<T>::type;
			constexpr component_key_t key = details::component_key<details::remove_all<component_type>>;

			auto& addedComponentsRange = m_StateChange[(m_Tick + 1) % 2].added_components[key];
			addedComponentsRange.insert(std::end(addedComponentsRange), std::begin(added_entities),
										std::end(added_entities));
		}

		template <typename T>
		void add_component(psl::array_view<entity> entities) noexcept
		{
			return add_component(entities, core::ecs::empty<T>{});
		}
	public:
		template <typename... Ts>
		void add_components(psl::array_view<entity> entities, Ts&&... args) noexcept
		{
			(add_component(entities, std::forward<Ts>(args)), ...);
		}

		template <typename... Ts>
		void add_components(psl::array_view<entity> entities) noexcept
		{
			(add_component<Ts>(entities), ...);
		}

		template <typename... Ts>
		void add_components(entity e, Ts&&... args) noexcept
		{
			(add_component(psl::array_view<entity>{&e, &e + 1}, std::forward<Ts>(args)), ...);
		}
		template <typename... Ts>
		void add_components(entity e) noexcept
		{
			(add_component<Ts>(psl::array_view<entity>{&e, &e + 1}), ...);
		}

		// -----------------------------------------------------------------------------
		// remove component
		// -----------------------------------------------------------------------------	private:
		template <typename T>
		void remove_component(psl::array_view<entity> entities) noexcept
		{
			auto removed_entities{details::remove_component<T>(m_EntityMap, m_Components, entities)};


			using component_type		  = typename details::get_component_type<T>::type;
			constexpr component_key_t key = details::component_key<details::remove_all<component_type>>;

			auto& removedComponentsRange = m_StateChange[(m_Tick + 1) % 2].removed_components[key];
			removedComponentsRange.insert(std::end(removedComponentsRange), std::begin(removed_entities),
										  std::end(removed_entities));
		}

		template <typename... Ts>
		void remove_components(psl::array_view<entity> entities) noexcept
		{
			(remove_component<Ts>(entities), ...);
		}


		template <typename T>
		void remove_component(entity e) noexcept
		{
			remove_component<T>(psl::array_view<entity>{&e, &e + 1});
		}

		template <typename... Ts>
		void remove_components(entity e) noexcept
		{
			(remove_component<Ts>(psl::array_view<entity>{&e, &e + 1}), ...);
		}


		// -----------------------------------------------------------------------------
		// create entities
		// -----------------------------------------------------------------------------


		template <typename... Ts>
		std::vector<entity> create(size_t count) noexcept
		{
			PROFILE_SCOPE(core::profiler)
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
				m_EntityMap.emplace(++mID, std::vector<std::pair<component_key_t, size_t>>{});
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components<Ts...>(result);
			}
			return result;
		}

		template <typename... Ts>
		std::vector<entity> create(size_t count, Ts&&... args) noexcept
		{
			PROFILE_SCOPE(core::profiler)
			m_EntityMap.reserve(m_EntityMap.size() + count);
			std::vector<entity> result(count);
			std::iota(std::begin(result), std::end(result), mID + 1);
			for(size_t i = 0u; i < count; ++i)
				m_EntityMap.emplace(++mID, std::vector<std::pair<component_key_t, size_t>>{});
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components(result, std::forward<Ts>(args)...);
			}
			return result;
		}

		template <typename... Ts>
		entity create_one(Ts&&... args) noexcept
		{
			PROFILE_SCOPE(core::profiler)
			auto e = m_EntityMap.emplace(entity{++mID}, std::vector<std::pair<component_key_t, size_t>>{}).first->first;
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components(e, std::forward<Ts>(args)...);
			}
			return e;
		}


		template <typename... Ts>
		entity create_one() noexcept
		{
			PROFILE_SCOPE(core::profiler)
			auto e = m_EntityMap.emplace(entity{++mID}, std::vector<std::pair<component_key_t, size_t>>{}).first->first;
			if constexpr(sizeof...(Ts) > 0)
			{
				add_components<Ts...>(e);
			}
			return e;
		}


		void destroy(psl::array_view<entity> entities) noexcept;
		void destroy(entity e) noexcept;


		// -----------------------------------------------------------------------------
		// filter on components
		// -----------------------------------------------------------------------------
		template <typename... Ts>
		std::vector<entity> filter() const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			static_assert(sizeof...(Ts) >= 1, "you should atleast have one component to filter on");
			return filter_foreach(std::make_index_sequence<sizeof...(Ts)>(), std::tuple<Ts...>());
		}

		template <typename... Ts>
		std::vector<entity> filter(std::vector<Ts>&... out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			auto entities{filter<Ts...>()};
			(fill_in(entities, out), ...);
			return entities;
		}

	  private:
		size_t prepare_bindings(psl::array_view<entity> entities, memory::raw_region& cache, size_t cache_offset,
								details::owner_dependency_pack& dep_pack) const noexcept;
		size_t prepare_data(psl::array_view<entity> entities, memory::raw_region& cache, size_t cache_offset,
							component_key_t id, size_t element_size) const noexcept;
		std::vector<core::ecs::command_buffer> prepare_system(std::chrono::duration<float> dTime, std::chrono::duration<float> rTime,
							memory::raw_region& cache, size_t cache_offset, details::system_information& system);

		std::vector<entity> filter(const details::owner_dependency_pack& pack) const
		{
			std::optional<std::vector<entity>> result{std::nullopt};

			auto merge = [](std::optional<std::vector<entity>> out,
							std::vector<entity> to_merge) -> std::vector<entity> {
				if(!out) return to_merge;

				std::vector<entity> v_intersection;
				v_intersection.reserve(std::max(out.value().size(), to_merge.size()));
				std::set_intersection(std::begin(out.value()), std::end(out.value()), std::begin(to_merge),
									  std::end(to_merge), std::back_inserter(v_intersection));

				return v_intersection;
			};

			auto difference = [](std::optional<std::vector<entity>> out,
								 std::vector<entity> to_merge) -> std::vector<entity> {
				if(!out) return to_merge;

				std::vector<entity> v_intersection;
				v_intersection.reserve(std::max(out.value().size(), to_merge.size()));
				std::set_difference(std::begin(out.value()), std::end(out.value()), std::begin(to_merge),
									std::end(to_merge), std::back_inserter(v_intersection));

				return v_intersection;
			};

			if(pack.filters.size() > 0)
			{
				result = merge(result, filter_default(pack.filters));
			}
			if(pack.on_add.size() > 0)
			{
				result = merge(result, filter_on_add(pack.on_add));
			}
			if(pack.on_remove.size() > 0)
			{
				result = merge(result, filter_on_remove(pack.on_remove));
			}
			if(pack.on_combine.size() > 0)
			{
				result = merge(result, filter_on_combine(pack.on_combine));
			}
			if(pack.on_break.size() > 0)
			{
				result = merge(result, filter_on_break(pack.on_break));
			}
			if(pack.except.size() > 0)
			{
				result = difference(result, filter_except(pack.except));
			}

			return result.value_or(std::vector<entity>{});
		}

		template <std::size_t... Is, typename... Ts>
		std::vector<entity> filter_foreach(std::index_sequence<Is...>, std::tuple<Ts...>) const
		{
			std::optional<std::vector<entity>> result{std::nullopt};
			auto merge = [](std::optional<std::vector<entity>> out,
							std::vector<entity> to_merge) -> std::vector<entity> {
				if(!out) return to_merge;

				std::vector<entity> v_intersection;
				v_intersection.reserve(std::max(out.value().size(), to_merge.size()));
				std::set_intersection(std::begin(out.value()), std::end(out.value()), std::begin(to_merge),
									  std::end(to_merge), std::back_inserter(v_intersection));

				return v_intersection;
			};

			auto difference = [](std::optional<std::vector<entity>> out,
								 std::vector<entity> to_merge) -> std::vector<entity> {
				if(!out) return to_merge;

				std::vector<entity> v_intersection;
				v_intersection.reserve(std::max(out.value().size(), to_merge.size()));
				std::set_difference(std::begin(out.value()), std::end(out.value()), std::begin(to_merge),
									std::end(to_merge), std::back_inserter(v_intersection));

				return v_intersection;
			};


			(std::invoke([&]() {
				 using component_t = typename std::tuple_element<Is, std::tuple<Ts...>>::type;
				 if constexpr(!details::is_exception<component_t>::value)
				 {
					 result = merge(result, filter_impl(component_t{}));
				 }
			 }),
			 ...);

			(std::invoke([&]() {
				 using component_t = typename std::tuple_element<Is, std::tuple<Ts...>>::type;
				 if constexpr(details::is_exception<component_t>::value)
				 {
					 result = difference(result, filter_impl(component_t{}));
				 }
			 }),
			 ...);

			return result.value_or(std::vector<entity>{});
		}

		std::vector<component_key_t>
		filter_keys(psl::array_view<component_key_t> keys,
					const details::key_value_container_t<component_key_t, std::vector<entity>>& key_list) const
		{
			std::vector<component_key_t> keys_out{};
			for(auto key : keys)
			{
				if(key_list.find(key) != std::end(key_list)) keys_out.emplace_back(key);
			}
			return keys;
		}

		std::vector<entity> filter_on_add(std::vector<component_key_t> keys) const
		{
			return dynamic_filter(keys, m_StateChange[m_Tick % 2].added_components);
		}

		std::vector<entity> filter_on_remove(std::vector<component_key_t> keys) const
		{
			return dynamic_filter(keys, m_StateChange[m_Tick % 2].removed_components);
		}

		std::vector<entity> filter_except(std::vector<component_key_t> keys) const { return dynamic_filter(keys); }

		std::vector<entity> filter_on_combine(std::vector<component_key_t> keys) const
		{
			std::sort(std::begin(keys), std::end(keys));
			std::vector<component_key_t> added_keys{filter_keys(keys, m_StateChange[m_Tick % 2].added_components)};
			if(added_keys.size() == 0) // at least 1 should be present
				return {};
			std::vector<component_key_t> remaining_keys{};

			std::set_difference(std::begin(keys), std::end(keys), std::begin(added_keys), std::end(added_keys),
								std::back_inserter(remaining_keys));

			auto entities = dynamic_filter(added_keys, m_StateChange[m_Tick % 2].added_components);
			if(remaining_keys.size() > 0) return dynamic_filter(remaining_keys, entities);

			return entities;
		}

		std::vector<entity> filter_on_break(std::vector<component_key_t> keys) const
		{
			std::sort(std::begin(keys), std::end(keys));
			std::vector<component_key_t> added_keys{filter_keys(keys, m_StateChange[m_Tick % 2].removed_components)};
			if(added_keys.size() == 0) // at least 1 should be present
				return {};
			std::vector<component_key_t> remaining_keys{};

			std::set_difference(std::begin(keys), std::end(keys), std::begin(added_keys), std::end(added_keys),
								std::back_inserter(remaining_keys));
			auto entities = dynamic_filter(added_keys, m_StateChange[m_Tick % 2].removed_components);
			if(remaining_keys.size() > 0) return dynamic_filter(remaining_keys, entities);

			return entities;
		}

		std::vector<entity> filter_default(std::vector<component_key_t> keys) const { return dynamic_filter(keys); }

		template <typename... Ts>
		std::vector<entity> filter_impl(on_add<Ts...>) const
		{
			std::vector<component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_on_add(keys);
		}

		template <typename... Ts>
		std::vector<entity> filter_impl(on_remove<Ts...>) const
		{
			std::vector<component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_on_remove(keys);
		}

		template <typename... Ts>
		std::vector<entity> filter_impl(except<Ts...>) const
		{
			std::vector<component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_except(keys);
		}


		template <typename... Ts>
		std::vector<entity> filter_impl(on_combine<Ts...>) const
		{
			std::vector<component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_on_combine(keys);
		}

		template <typename... Ts>
		std::vector<entity> filter_impl(on_break<Ts...>) const
		{
			std::vector<component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			return filter_on_break(keys);
		}

		template <typename T>
		std::vector<entity> filter_impl(T) const
		{
			std::vector<component_key_t> keys{{details::component_key<details::remove_all<T>>}};
			return filter_default(keys);
		}


	  public:
		// -----------------------------------------------------------------------------
		// get component
		// -----------------------------------------------------------------------------
		template <typename T>
		T& get_component(entity e)
		{
			PROFILE_SCOPE(core::profiler)
			constexpr component_key_t int_id = details::component_key<details::remove_all<T>>;
			auto eMapIt						 = m_EntityMap.find(e);
			auto foundIt					 = std::find_if(
				eMapIt->second.begin(), eMapIt->second.end(),
				[&int_id](const std::pair<component_key_t, size_t>& pair) { return pair.first == int_id; });

			if(foundIt == std::end(eMapIt->second))
			{
				throw std::runtime_error("missing component");
			}
			const auto& index	= foundIt->second;
			const auto& mem_pair = m_Components.find(int_id);
			return *(T*)((std::uintptr_t)mem_pair->second.region.data() + mem_pair->second.size * index);
		}

		template <typename T>
		const T& get_component(entity e) const
		{
			PROFILE_SCOPE(core::profiler)
			constexpr component_key_t int_id = details::component_key<details::remove_all<T>>;
			auto eMapIt						 = m_EntityMap.find(e);
			auto foundIt					 = std::find_if(
				eMapIt->second.begin(), eMapIt->second.end(),
				[&int_id](const std::pair<component_key_t, size_t>& pair) { return pair.first == int_id; });

			if(foundIt == std::end(eMapIt->second))
			{
				throw std::runtime_error("missing component");
			}
			const auto& index	= foundIt->second;
			const auto& mem_pair = m_Components.find(int_id);
			return *(T*)((std::uintptr_t)mem_pair->second.region.data() + mem_pair->second.size * index);
		}

		// -----------------------------------------------------------------------------
		// systems
		// -----------------------------------------------------------------------------
		void tick(std::chrono::duration<float> dTime = std::chrono::duration<float>{0.0f});

		template <typename T>
		void set(psl::array_view<entity> entities, psl::array_view<T> data)
		{
			PROFILE_SCOPE(core::profiler)
			constexpr component_key_t id = details::component_key<details::remove_all<T>>;
			const auto& mem_pair		 = m_Components.find(id);
			auto size					 = sizeof(T);

			for(const auto& [i, e] : psl::enumerate(entities))
			{
				auto eMapIt = m_EntityMap.find(e);
				auto foundIt =
					std::find_if(eMapIt->second.begin(), eMapIt->second.end(),
								 [&id](const std::pair<component_key_t, size_t>& pair) { return pair.first == id; });

				auto index = foundIt->second;
				void* loc  = (void*)((std::uintptr_t)mem_pair->second.region.data() + size * index);
				std::memcpy(loc, &data[i], size);
			}
		}

		bool exists(entity e) const noexcept { return m_EntityMap.find(e) != std::end(m_EntityMap); }
		template <typename T>
		bool has_component(entity e) const noexcept
		{
			constexpr component_key_t key = details::component_key<details::remove_all<T>>;
			if(auto eIt = m_EntityMap.find(e); eIt != std::end(m_EntityMap))
			{
				return std::any_of(
					std::begin(eIt->second), std::end(eIt->second),
					[&key](const std::pair<component_key_t, size_t> comp_pair) { return comp_pair.first == key; });
			}

			return false;
		}

		template <typename... Ts>
		bool has_components(entity e) const noexcept
		{
			std::vector<component_key_t> keys{{details::component_key<details::remove_all<Ts>>...}};
			if(auto eIt = m_EntityMap.find(e); eIt != std::end(m_EntityMap))
			{
				return std::all_of(std::begin(keys), std::end(keys), [&eIt](const component_key_t& key) {
					return std::any_of(
						std::begin(eIt->second), std::end(eIt->second),
						[&key](const std::pair<component_key_t, size_t> comp_pair) { return comp_pair.first == key; });
				});
			}
			return false;
		}


		// will return false when the entity does not exist either
		template <typename T>
		bool is_owned_by(entity e, const T& component)
		{
			constexpr component_key_t key = details::component_key<details::remove_all<T>>;
			if(auto eIt = m_EntityMap.find(e); eIt != std::end(m_EntityMap))
			{
				if(auto compIt = std::find_if(
					   std::begin(eIt->second), std::end(eIt->second),
					   [&key](const std::pair<component_key_t, size_t> comp_pair) { return comp_pair.first == key; });
				   compIt != std::end(eIt->second))
				{
					auto compDataIt = m_Components.find(key);
					auto diff		= &component - (T*)compDataIt->second.region.data();
					return compIt->second == diff;
				}
				return false;
			}
			return false;
		}

		template <typename T, typename... Ts>
		bool is_owned_by(entity e, const T& component, const Ts&... components)
		{
			return is_owned_by(e, std::forward<T>(component)) && is_owned_by(e, std::forward<Ts>(components)...);
		}

	  private:
		template <typename... Ts>
		struct get_packs
		{
			using type = std::tuple<Ts...>;
		};

		template <typename... Ts>
		struct get_packs<std::tuple<Ts...>> : public get_packs<Ts...>
		{};

		template <typename... Ts>
		struct get_packs<const core::ecs::state&, std::chrono::duration<float>, std::chrono::duration<float>, Ts...>
			: public get_packs<Ts...>
		{};


		template <typename Fn, typename T = void>
		void declare_impl(threading threading, Fn&& fn, T* ptr = nullptr)
		{

			using function_args = typename psl::templates::func_traits<typename std::decay<Fn>::type>::arguments_t;
			std::function<std::vector<details::owner_dependency_pack>()> pack_generator = []() {
				using pack_t = typename get_packs<function_args>::type;
				return details::expand_to_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
														  details::type_container<pack_t>{});
			};

			std::function<core::ecs::command_buffer(const core::ecs::state&, std::chrono::duration<float>,
													std::chrono::duration<float>,
													std::vector<details::owner_dependency_pack>)>
				system_tick;

			if constexpr(std::is_member_function_pointer<Fn>::value)
			{
				system_tick = [fn,
							   ptr](const core::ecs::state& state, std::chrono::duration<float> dTime,
									std::chrono::duration<float> rTime,
									std::vector<details::owner_dependency_pack> packs) -> core::ecs::command_buffer {
					using pack_t = typename get_packs<function_args>::type;

					auto tuple_argument_list = std::tuple_cat(
						std::tuple<T*, const core::ecs::state&, std::chrono::duration<float>,
								   std::chrono::duration<float>>(ptr, state, dTime, rTime),
						details::compress_from_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
															   details::type_container<pack_t>{}, packs));

					return std::apply(fn, std::move(tuple_argument_list));
				};
			}
			else
			{
				system_tick = [fn](const core::ecs::state& state, std::chrono::duration<float> dTime,
								   std::chrono::duration<float> rTime,
								   std::vector<details::owner_dependency_pack> packs) -> core::ecs::command_buffer {
					using pack_t = typename get_packs<function_args>::type;

					auto tuple_argument_list = std::tuple_cat(
						std::tuple<const core::ecs::state&, std::chrono::duration<float>, std::chrono::duration<float>>(
							state, dTime, rTime),
						details::compress_from_dependency_pack(std::make_index_sequence<std::tuple_size_v<pack_t>>{},
															   details::type_container<pack_t>{}, packs));

					return std::apply(fn, std::move(tuple_argument_list));
				};
			}
			m_SystemInformations.emplace_back(details::system_information{threading, pack_generator, system_tick});
		}


	  public:
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
		void set(psl::array_view<entity> entities, void* data, size_t size, component_key_t id) const noexcept;
		std::vector<entity> dynamic_filter(psl::array_view<component_key_t> keys,
										   std::optional<psl::array_view<entity>> pre_selection = std::nullopt) const
			noexcept;
		std::vector<entity>
		dynamic_filter(psl::array_view<component_key_t> keys,
					   const details::key_value_container_t<component_key_t, std::vector<entity>>& container,
					   std::optional<psl::array_view<entity>> pre_selection = std::nullopt) const noexcept;
		void fill_in(psl::array_view<entity> entities, component_key_t int_id, void* out) const noexcept;

		template <typename T>
		void fill_in(psl::array_view<entity> entities, std::vector<T>& out) const noexcept
		{
			PROFILE_SCOPE(core::profiler)
			out.resize(entities.size());
			return fill_in(entities, details::component_key<details::remove_all<T>>, out.data());
		}

		void execute_command_buffer(command_buffer& cmds);

		uint64_t mID{0u};

		std::vector<details::system_information> m_SystemInformations;
		/// \brief gets what components this entity uses, and which index it lives on.
		details::key_value_container_t<entity, std::vector<std::pair<component_key_t, size_t>>> m_EntityMap;

		/// \brief backing memory
		details::key_value_container_t<component_key_t, details::component_info> m_Components;
		/// overhead is
		/// sizeof(component) * Nc + sizeof(entity) * Ne +
		/// // store ever component as well as entity (sizeof(entity) + ((sizeof(size_t) /* component ID */ +
		/// sizeof(size_t) /* component index*/) * Nce)) * Ne +	// for every entity store the component ID, and index
		/// that it uses (sizeof(size_t) /* component ID */ + sizeof(entity) * Nec) * nct
		/// // for every component, store the entity that uses them.
		///
		/// where Nc is the count of unique components that exists (not types)
		/// where Ne is the count of the unique entities that exists
		/// where Nce is the count of components this entity has
		/// where Nec is the count of entities that uses this component type.
		/// where Nct is the count of unique component types.

		memory::raw_region m_Cache{1024 * 1024 * 32};

		// keep track of changed data
		struct difference_set
		{
			void clear()
			{
				added_entities.clear();
				removed_entities.clear();
				added_components.clear();
				removed_components.clear();
			}
			std::vector<entity> added_entities;
			std::vector<entity> removed_entities;
			details::key_value_container_t<component_key_t, std::vector<entity>> added_components;
			details::key_value_container_t<component_key_t, std::vector<entity>> removed_components;
		};

		//memory::region m_Region{1024* 1024* 1024, 4};
		psl::async::scheduler* m_Scheduler{nullptr};
		std::array<difference_set, 2> m_StateChange;
		size_t m_Tick{0};

		// std::unordered_map<component_key_t,
	};
} // namespace core::ecs