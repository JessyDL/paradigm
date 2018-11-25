#include "stdafx.h"
#include "ecs/systems/geometry_instance.h"
#include "gfx/material.h"
#include "vk/geometry.h"
#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "ecs/components/input_tag.h"
#include "conversion_utils.h"

using namespace core::resource;
using namespace core::gfx;
using namespace core::os;
using namespace core;
using namespace core::ecs;
using namespace core::ecs::systems;
using namespace core::ecs::components;
using namespace psl::math;

template<typename T>
struct on_add{};
template<typename T>
struct on_remove{};
template<typename T>
struct on_modified{};

template<typename T>
struct except_add {};
template<typename T>
struct except_modified {};

namespace details
{
	template<bool has_entities, typename... Ts>
	struct pack_tuple
	{
		using range_t = std::tuple<core::ecs::vector<entity>, core::ecs::vector<Ts>...>;
		using range_element_t = std::tuple<entity, Ts...>;
	};

	template<typename... Ts>
	struct pack_tuple<false, Ts...>
	{
		using range_t = std::tuple<core::ecs::vector<Ts>...>;
		using range_element_t = std::tuple<Ts...>;
	};
	
	template <typename T, typename Tuple>
	struct has_type;

	template <typename T>
	struct has_type<T, std::tuple<>> : std::false_type {};

	template <typename T, typename U, typename... Ts>
	struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>> {};

	template <typename T, typename... Ts>
	struct has_type<T, std::tuple<T, Ts...>> : std::true_type {};

	template <typename T, typename Tuple>
	using tuple_contains_type = typename has_type<T, Tuple>::type;
}

template <bool has_entities, typename... Ts>
class tpack
{
	using range_t = typename ::details::pack_tuple<has_entities, Ts...>::range_t;
	using range_element_t = typename ::details::pack_tuple<has_entities, Ts...>::range_element_t;
	using iterator_element_t =
		std::tuple<typename core::ecs::vector<entity>::iterator, typename core::ecs::vector<Ts>::iterator...>;

	template <typename Tuple, typename F, std::size_t... Indices>
	static void for_each_impl(Tuple&& tuple, F&& f, std::index_sequence<Indices...>)
	{
		using swallow = int[];
		(void)swallow
		{
			1, (f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...
		};
	}

	template <typename Tuple, typename F>
	static void for_each(Tuple&& tuple, F&& f)
	{
		constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
		for_each_impl(std::forward<Tuple>(tuple), std::forward<F>(f), std::make_index_sequence<N>{});
	}

public:
	class iterator
	{
	public:
		constexpr iterator(const range_t& range) noexcept {};
		constexpr iterator(iterator_element_t data) noexcept : data(data) {};
		constexpr const iterator_element_t& operator*() const noexcept { return data; }
		constexpr bool operator!=(iterator other) const noexcept
		{
			return std::get<0>(data) != std::get<0>(other.data);
		}
		constexpr iterator operator++() const noexcept
		{
			auto next = iterator(data);
			++next;
			return next;
		}
		constexpr iterator& operator++() noexcept
		{
			for_each(data, [](auto& element) { ++element; });
			return *this;
		}

	private:
		iterator_element_t data;
	};
	range_t read() { return m_Pack; }

	template <typename T>
	const core::ecs::vector<T>& get()
	{
		static_assert(::details::tuple_contains_type<core::ecs::vector<T>, range_t>::value, "the requested component type does not exist in the pack");
		return std::get<core::ecs::vector<T>>(m_Pack);
	}

	iterator begin() const noexcept { return iterator{m_Pack}; }

	iterator end() const noexcept { return iterator{m_Pack}; }
	constexpr size_t size() const noexcept { return std::get<0>(m_Pack).size(); }

private:
	range_t m_Pack;
};


geometry_instance::geometry_instance(core::ecs::state& state)
{
	state.register_system(*this);
	state.register_dependency(*this, {m_Entities, m_Transforms, m_Renderers, m_Velocity});
	state.register_dependency(*this, {m_LifeEntities, m_Lifetime});
	state.register_dependency(*this, {m_CamEntities, m_CamTransform, core::ecs::filter<core::ecs::components::input_tag>{}});
}

float accTime {0.0f};
void geometry_instance::tick(core::ecs::state& state, std::chrono::duration<float> dTime)
{
	//tpack<true, const transform, const renderable> p{};
	//auto pAck = p.read();
	//auto transformPack = p.get<const transform>();
	PROFILE_SCOPE(core::profiler)
	accTime += dTime.count();

	std::vector<entity> dead_ents;
	for(size_t i = 0; i < m_LifeEntities.size(); ++i)
	{
		m_Lifetime[i].value -= dTime.count();
		if(m_Lifetime[i].value <= 0.0f)
			dead_ents.emplace_back(m_LifeEntities[i]);
	}
	state.add_component<dead_tag>(dead_ents);

	core::profiler.scope_begin("release material handles");
	for(const auto& renderer : m_Renderers)
	{
		renderer.material.handle()->release_all();
	}
	core::profiler.scope_end();
	core::profiler.scope_begin("rotate and reposition all transforms");

	//for(size_t i = 0; i < m_Entities.size(); ++i)
	//{
	//	auto mag = magnitude(m_Transforms[i].position - m_CamTransform[0].position);
	//	m_Transforms[i].position += (normalize(m_Transforms[i].position) * dTime.count() * 3.0f * sin(accTime*0.1f));
	//	m_Transforms[i].rotation = normalize(psl::quat(0.8f* dTime.count() * saturate((mag - 6)*0.1f), 0.0f, 0.0f, 1.0f) * m_Transforms[i].rotation);
	//	if(mag < 6)
	//	{
	//		m_Transforms[i].rotation = normalize(psl::math::look_at_q(m_Transforms[i].position, m_CamTransform[0].position, psl::vec3::up));
	//	}

	//	//
	//	//m_Transforms[i].rotation = normalize(m_CamTransform[0].rotation);
	//}
	for(size_t i = 0; i < m_Entities.size(); ++i)
	{
		m_Transforms[i].position += m_Velocity[i].direction * m_Velocity[i].force * dTime.count();
		const auto mag = magnitude(m_Transforms[i].position - m_CamTransform[0].position);
		m_Transforms[i].rotation = normalize(psl::quat(0.8f* dTime.count() * saturate((mag - 6)*0.1f), 0.0f, 0.0f, 1.0f) * m_Transforms[i].rotation);
	}
	core::profiler.scope_end();
	
	ska::bytell_hash_map<psl::UID, ska::bytell_hash_map<psl::UID, std::vector<size_t>>> UniqueCombinations;
	for(size_t i = 0; i < m_Entities.size(); ++i)
	{
		UniqueCombinations[m_Renderers[i].material][m_Renderers[i].geometry].emplace_back(i);
	}

	std::vector<psl::mat4x4> modelMats;
	for(const auto& uniqueCombination : UniqueCombinations)
	{
		for(const auto& uniqueIndex : uniqueCombination.second)
		{
			if(uniqueIndex.second.size() == 0)
				continue;

			modelMats.clear();
			auto materialHandle = m_Renderers[uniqueIndex.second[0]].material.handle();
			auto geometryHandle = m_Renderers[uniqueIndex.second[0]].geometry.handle();

			uint32_t startIndex = 0u;
			uint32_t indexCount = 0u;
			bool setStart = true;
			core::profiler.scope_begin("generate instance data");
			for(size_t i : uniqueIndex.second)
			{
				if(auto instanceID = materialHandle->instantiate(geometryHandle); instanceID)
				{
					if(!setStart && (instanceID.value() - indexCount != startIndex))
					{
						materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
						modelMats.clear();
						startIndex = instanceID.value();
						indexCount = 0;
					}

					if(setStart)
					{
						startIndex = instanceID.value();
					}

					++indexCount;
					setStart = false;
					const psl::mat4x4 translationMat = translate(m_Transforms[i].position);
					const psl::mat4x4 rotationMat = to_matrix(m_Transforms[i].rotation);
					const psl::mat4x4 scaleMat = scale(m_Transforms[i].scale);

					modelMats.emplace_back(translationMat * rotationMat * scaleMat);
				}
			}
			core::profiler.scope_end();
			core::profiler.scope_begin("sending new instance data to GPU");
			if(modelMats.size() > 0)
				materialHandle->set(geometryHandle, startIndex, "INSTANCE_TRANSFORM", modelMats);
			core::profiler.scope_end();
		}		
	}	
}