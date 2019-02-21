#pragma once
#include "component_key.h"
#include <functional>
#include "array_view.h"
#include <cstdint>
#include "entity.h"
#include "component_info.h"
#include "component_key.h"
#include "bytell_map.h"
#include <optional>

namespace core::ecs::details
{
	template <typename KeyT, typename ValueT>
	using key_value_container_t = psl::bytell_map<KeyT, ValueT>;
	//using key_value_container_t = std::unordered_map<KeyT, ValueT>;


	template <typename T>
	struct is_tag : std::false_type
	{
		using type = T;
	};

	template <typename T>
	struct is_tag<empty<T>> : std::true_type
	{
		using type = T;
	};

	template <typename T, typename SFINEA = void>
	struct get_component_type
	{
		using type = typename std::invoke_result<T, size_t>::type;
	};
	template <typename T>
	struct get_component_type<T, typename std::enable_if<!std::is_invocable<T, size_t>::value>::type>
	{
		using type = typename core::ecs::details::is_tag<T>::type;
	};

	template <typename T, typename SFINEA = void>
	struct get_forward_type
	{
		using type = T;
	};
	template <typename T>
	struct get_forward_type<T, typename std::enable_if<!std::is_invocable<T, size_t>::value>::type>
	{
		using type = typename core::ecs::details::is_tag<T>::type;
	};

	template <typename T>
	typename std::enable_if<!std::is_invocable<T, size_t>::value>::type
		initialize_component(void* location, psl::array_view<size_t> indices, T&& data) noexcept
	{
		for(size_t i = 0; i < indices.size(); ++i)
		{
			std::memcpy((void*)((std::uintptr_t)location + indices[i] * sizeof(T)), &data, sizeof(T));
		}
	}

	template <typename T>
	typename std::enable_if<std::is_invocable<T, size_t>::value>::type
		initialize_component(void* location, psl::array_view<size_t> indices, T&& invokable) noexcept
	{
		constexpr size_t size = sizeof(typename std::invoke_result<T, size_t>::type);
		for(size_t i = 0; i < indices.size(); ++i)
		{
			auto v{std::invoke(invokable, i)};
			std::memcpy((void*)((std::uintptr_t)location + indices[i] * size), &v, size);
		}
	}

	template <typename T>
	std::vector<entity> duplicate_component_check(
		psl::array_view<entity> entities,
		const details::key_value_container_t<entity, entity_data>&
		entityMap)
	{
		constexpr component_key_t key = details::component_key<details::remove_all<T>>;
		std::vector<entity> ent_cpy/* = entities*/;
		auto end = std::remove_if(std::begin(ent_cpy), std::end(ent_cpy), [&entityMap, key](const entity& e)
								  {
									  auto eMapIt = entityMap.find(e);
									  if(eMapIt == std::end(entityMap))
									  {
										  return true;
									  }

									  for(auto eComp : eMapIt->second.components)
									  {
										  if(eComp == key) return true;
									  }
									  return false;
								  });
		ent_cpy.resize(std::distance(std::begin(ent_cpy), end));
		return ent_cpy;
	}


	static component_info& get_component_info(
		component_key_t key, size_t component_size,
		details::key_value_container_t<component_key_t, details::component_info>& components)
	{
		auto it = components.find(key);
		if(it == components.end())
		{
			if(component_size == 0)
			{
				components.emplace(key, details::component_info{key});
			}
			else
			{
				components.emplace(
					key, details::component_info{key, component_size});
			}
			it = components.find(key);
		}
		return it->second;
	}

	template <typename T>
	static component_info& get_component_info(
		details::key_value_container_t<component_key_t, details::component_info>& components)
	{
		constexpr auto key = details::component_key<details::remove_all<T>>;
		auto it = components.find(key);
		if(it == components.end())
		{
			if(std::is_empty<T>::value)
			{
				components.emplace(key, details::component_info{key});
			}
			else
			{
				components.emplace(
					key, details::component_info{key, sizeof(T)});
			}
			it = components.find(key);
		}
		return it->second;
	}

	template <typename T>
	std::vector<entity> add_component(
		details::key_value_container_t<entity, entity_data>& entityMap,
		details::key_value_container_t<component_key_t, details::component_info>& components,
		psl::array_view<entity> entities, T&& _template) noexcept
	{
		using component_type = typename get_component_type<T>::type;
		using forward_type = typename get_forward_type<T>::type;


		static_assert(
			std::is_trivially_copyable<component_type>::value && std::is_standard_layout<component_type>::value,
			"the component type must be trivially copyable and standard layout "
			"(std::is_trivially_copyable<T>::value == true && std::is_standard_layout<T>::value == true)");
		constexpr component_key_t key = details::component_key<details::remove_all<component_type>>;

		std::vector<entity> ent_cpy = duplicate_component_check<component_type>(entities, entityMap);
		std::sort(std::begin(ent_cpy), std::end(ent_cpy));

		auto& componentInfo{get_component_info<component_type>(components)};

		if(componentInfo.available() <= ent_cpy.size())
			componentInfo.resize(std::max(componentInfo.capacity * 2, componentInfo.capacity + ent_cpy.size()));

		entityMap.reserve(entityMap.size() + std::distance(std::begin(ent_cpy), std::end(ent_cpy)));

		if constexpr(std::is_empty<component_type>::value)
		{
			for(auto ent_it = std::begin(ent_cpy); ent_it != std::end(ent_cpy); ++ent_it)
			{
				const entity& e{*ent_it};
				entityMap[e].emplace_back(key, 0);
				componentInfo.entities.emplace(
					std::upper_bound(std::begin(componentInfo.entities), std::end(componentInfo.entities), e), e);
			}
		}
		else
		{
			std::vector<uint64_t> indices;
			const auto count = std::distance(std::begin(ent_cpy), std::end(ent_cpy));
			indices.reserve(count);
			entityMap.reserve(entityMap.size() + count);


			std::vector<entity> merged;
			merged.reserve(componentInfo.entities.size() + count);
			std::merge(std::begin(componentInfo.entities), std::end(componentInfo.entities), std::begin(ent_cpy),
					   std::end(ent_cpy), std::back_inserter(merged));
			componentInfo.entities = std::move(merged);

			auto id_ranges = componentInfo.generator.create_multi(count);
			auto ent_it = std::begin(ent_cpy);
			for(const auto& id_range : id_ranges)
			{
				for(auto i = id_range.first; i < id_range.second; ++i)
				{
					const entity& e{*ent_it};
					indices.emplace_back(i);
					entityMap[e].emplace_back(key, i);
					ent_it = std::next(ent_it);
				}
			}

			if constexpr(core::ecs::details::is_tag<T>::value)
			{
				if constexpr(!std::is_trivially_constructible<component_type>::value)
				{
					component_type v{};
					initialize_component(componentInfo.region.data(), indices, std::move(v));
				}
			}
			else
			{
				initialize_component(componentInfo.region.data(), indices, std::forward<forward_type>(_template));
			}
		}

		return ent_cpy;
	}

	static std::vector<entity> remove_component(component_key_t key,
		details::key_value_container_t<entity, entity_data>& entityMap,
		details::key_value_container_t<component_key_t, details::component_info>& components,
		psl::array_view<entity> entities) noexcept
	{
		std::vector<entity> ent_cpy;
		ent_cpy.reserve(entities.size());
		for(auto e : entities)
		{
			auto eMapIt = entityMap.find(e);
			auto foundIt = std::find(std::begin(eMapIt->second.components), std::end(eMapIt->second.components), key);

			if(foundIt == std::end(eMapIt->second.components)) continue;

			ent_cpy.emplace_back(e);
			const auto& eCompIt = components.find(key);
			eCompIt->second.entities.erase(
				std::remove(eCompIt->second.entities.begin(), eCompIt->second.entities.end(), e),
				eCompIt->second.entities.end());

			eCompIt->second.removed_entities.emplace(std::upper_bound(std::begin(eCompIt->second.removed_entities), std::end(eCompIt->second.removed_entities), e), e);
		}

		return ent_cpy;
	}

	template <typename T>
	std::vector<entity> remove_component(
		details::key_value_container_t<entity, entity_data>& entityMap,
		details::key_value_container_t<component_key_t, details::component_info>& components,
		psl::array_view<entity> entities) noexcept
	{
		constexpr component_key_t key = details::component_key<details::remove_all<T>>;
		return remove_component(key, entityMap, components, entities);
	}
}