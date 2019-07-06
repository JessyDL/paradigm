#pragma once
#include <functional>
#include "component_key.h"
#include "../command_buffer.h"
#include "template_utils.h"
#include "array_view.h"
#include "../pack.h"
#include <chrono>

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

	class state;

	struct info
	{
		info(const state& state, std::chrono::duration<float> dTime, std::chrono::duration<float> rTime)
			: state(state), dTime(dTime), rTime(rTime), command_buffer(state){};

		const state& state;
		command_buffer command_buffer;
		std::chrono::duration<float> dTime;
		std::chrono::duration<float> rTime;
	};
} // namespace psl::ecs

namespace psl::ecs::details
{
	template <typename T, typename Fn>
	void insertion_sort(T&& first, T&& last, Fn&& pred)
	{
		/*for(auto it = std::next(first); it != last; ++it)
		{
			auto curr = it;
			while(curr != first && std::invoke(pred, *it, *std::prev(curr)))
			{
				curr = std::prev(curr);
			}
			std::swap(*curr, *it);
		}*/
	}

	template <typename Sorter, typename... Ts>
	struct sorting
	{
		template <typename... Ys>
		bool operator()(const psl::tuple_ref<Ys...>& lhs, const psl::tuple_ref<Ys...>& rhs) const noexcept
		{
			return false;
		}
	};

	/// \brief describes a set of dependencies for a given system
	///
	/// systems can have various dependencies, for example a movement system could have
	/// dependencies on both a psl::ecs::components::transform component and a psl::ecs::components::renderable
	/// component. This dependency will output a set of psl::ecs::entity's that have all required
	/// psl::ecs::components present. Certain systems could have sets of dependencies, for example the render
	/// system requires knowing about both all `psl::ecs::components::renderable` that have a
	/// `psl::ecs::components::transform`, but also needs to know all `psl::ecs::components::camera's`. So that
	/// system would require several dependency_pack's.
	class dependency_pack
	{
		friend class psl::ecs::state;
		template <std::size_t... Is, typename T>
		auto create_dependency_filters(std::index_sequence<Is...>, psl::templates::type_container<T>)
		{
			(add(psl::templates::type_container<
				 typename std::remove_reference<decltype(std::declval<T>().template get<Is>())>::type>{}),
			 ...);
		}

		template <typename F>
		void select_impl(std::vector<component_key_t>& target)
		{
			if constexpr(!std::is_same<typename std::decay<F>::type, psl::ecs::entity>::value)
			{
				using component_t			  = F;
				constexpr component_key_t key = details::key_for<component_t>();
				target.emplace_back(key);
				m_Sizes[key] = sizeof(component_t);
			}
		}

		template <std::size_t... Is, typename T>
		auto select(std::index_sequence<Is...>, T, std::vector<component_key_t>& target)
		{
			(select_impl<typename std::tuple_element<Is, T>::type>(target), ...);
		}

		template <typename Pred, typename... Ts>
		void select_ordering_impl(std::pair<Pred, std::tuple<Ts...>>);


		template <typename Pred, typename... Ts>
		void select_condition_impl(std::pair<Pred, std::tuple<Ts...>>);


		template <typename... Ts>
		void select_ordering(std::tuple<Ts...>)
		{
			(select_ordering_impl(Ts{}), ...);
		}

		template <typename... Ts>
		void select_condition(std::tuple<Ts...>)
		{
			(select_condition_impl(Ts{}), ...);
		}

		template <typename T>
		psl::array_view<T> fill_in(psl::templates::type_container<psl::array_view<T>>)
		{
			if constexpr(std::is_same<T, psl::ecs::entity>::value)
			{
				return m_Entities;
			}
			else if constexpr(std::is_const<T>::value)
			{
				constexpr component_key_t int_id = details::key_for<T>();
				return *(psl::array_view<T>*)&m_RBindings[int_id];
			}
			else
			{
				constexpr component_key_t int_id = details::key_for<T>();
				return *(psl::array_view<T>*)&m_RWBindings[int_id];
			}
		}

		template <std::size_t... Is, typename T>
		T to_pack_impl(std::index_sequence<Is...>, psl::templates::type_container<T>)
		{
			using pack_t	  = T;
			using pack_view_t = typename pack_t::pack_t;
			using range_t	 = typename pack_t::pack_t::range_t;

			return T{pack_view_t(
				fill_in(psl::templates::type_container<typename std::tuple_element<Is, range_t>::type>())...)};
		}


	  public:
		template <typename T>
		dependency_pack(psl::templates::type_container<T>, bool seedWithPrevious = false)
		{
			orderby		 = [](psl::array<entity>::iterator begin, psl::array<entity>::iterator end,
						  const psl::ecs::state& state) {};
			using pack_t = T;
			create_dependency_filters(std::make_index_sequence<std::tuple_size_v<typename pack_t::pack_t::range_t>>{},
									  psl::templates::type_container<T>{});
			select(std::make_index_sequence<std::tuple_size<typename pack_t::filter_t>::value>{},
				   typename pack_t::filter_t{}, filters);
			select(std::make_index_sequence<std::tuple_size<typename pack_t::add_t>::value>{}, typename pack_t::add_t{},
				   (seedWithPrevious) ? filters : on_add);
			select(std::make_index_sequence<std::tuple_size<typename pack_t::remove_t>::value>{},
				   typename pack_t::remove_t{}, on_remove);
			select(std::make_index_sequence<std::tuple_size<typename pack_t::break_t>::value>{},
				   typename pack_t::break_t{}, on_break);
			select(std::make_index_sequence<std::tuple_size<typename pack_t::combine_t>::value>{},
				   typename pack_t::combine_t{}, (seedWithPrevious) ? filters : on_combine);
			select(std::make_index_sequence<std::tuple_size<typename pack_t::except_t>::value>{},
				   typename pack_t::except_t{}, except);
			select_ordering(typename pack_t::order_by_t{});
			select_condition(typename pack_t::conditional_t{});

			static_assert(std::tuple_size<typename pack_t::order_by_t>::value < 2);

			std::sort(std::begin(filters), std::end(filters));
			filters.erase(std::unique(std::begin(filters), std::end(filters)), std::end(filters));

			if constexpr(std::is_same<psl::ecs::partial, typename pack_t::policy_t>::value)
			{
				m_IsPartial = true;
			}
		};


		~dependency_pack() noexcept						  = default;
		dependency_pack(const dependency_pack& other)	 = default;
		dependency_pack(dependency_pack&& other) noexcept = default;
		dependency_pack& operator=(const dependency_pack&) = default;
		dependency_pack& operator=(dependency_pack&&) noexcept = default;


		template <typename... Ts>
		psl::ecs::pack<Ts...> to_pack(psl::templates::type_container<psl::ecs::pack<Ts...>>)
		{
			using pack_t  = psl::ecs::pack<Ts...>;
			using range_t = typename pack_t::pack_t::range_t;

			return to_pack_impl(std::make_index_sequence<std::tuple_size<range_t>::value>{},
								psl::templates::type_container<pack_t>{});
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

		template <typename T>
		size_t size_of() const noexcept
		{
			constexpr component_key_t int_id = details::key_for<T>();
			return m_Sizes.at(int_id);
		}

		size_t size_of(component_key_t key) const noexcept
		{
			return m_Sizes.at(key);
		}

		size_t entities() const noexcept { return m_Entities.size(); }
		dependency_pack slice(size_t begin, size_t end) const noexcept
		{
			dependency_pack cpy{*this};

			cpy.m_Entities = cpy.m_Entities.slice(begin, end);

			for(auto& binding : cpy.m_RBindings)
			{
				auto size = cpy.m_Sizes[binding.first];

				std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
				std::uintptr_t end_mem   = (std::uintptr_t)binding.second.data() + (end * size);
				binding.second = psl::array_view<std::uintptr_t>{(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
			}
			for(auto& binding : cpy.m_RWBindings)
			{
				auto size = cpy.m_Sizes[binding.first];
				// binding.second = binding.second.slice(size * begin, size * end);
				std::uintptr_t begin_mem = (std::uintptr_t)binding.second.data() + (begin * size);
				std::uintptr_t end_mem   = (std::uintptr_t)binding.second.data() + (end * size);
				binding.second = psl::array_view<std::uintptr_t>{(std::uintptr_t*)begin_mem, (std::uintptr_t*)end_mem};
			}
			return cpy;
		}

	  private:
		template <typename T>
		void add(psl::templates::type_container<psl::array_view<T>>) noexcept
		{
			constexpr component_key_t int_id = details::key_for<T>();
			m_RWBindings.emplace(int_id, psl::array_view<std::uintptr_t>{});
		}

		template <typename T>
		void add(psl::templates::type_container<psl::array_view<const T>>) noexcept
		{
			constexpr component_key_t int_id = details::key_for<T>();
			m_RBindings.emplace(int_id, psl::array_view<std::uintptr_t>{});
		}


		void add(psl::templates::type_container<psl::array_view<psl::ecs::entity>>) noexcept {}
		void add(psl::templates::type_container<psl::array_view<const psl::ecs::entity>>) noexcept {}

	  private:
		psl::array_view<psl::ecs::entity> m_Entities{};
		std::unordered_map<component_key_t, size_t> m_Sizes;
		std::unordered_map<component_key_t, psl::array_view<std::uintptr_t>> m_RBindings;
		std::unordered_map<component_key_t, psl::array_view<std::uintptr_t>> m_RWBindings;

		std::vector<component_key_t> filters;
		std::vector<component_key_t> on_add;
		std::vector<component_key_t> on_remove;
		std::vector<component_key_t> except;
		std::vector<component_key_t> on_combine;
		std::vector<component_key_t> on_break;

		std::vector<std::function<psl::array<entity>::iterator(psl::array<entity>::iterator,
															   psl::array<entity>::iterator, const psl::ecs::state&)>>
			on_condition;

		std::function<void(psl::array<entity>::iterator, psl::array<entity>::iterator, const psl::ecs::state&)> orderby;
		bool m_IsPartial = false;
	};

	template <std::size_t... Is, typename T>
	std::vector<dependency_pack> expand_to_dependency_pack(std::index_sequence<Is...>,
														   psl::templates::type_container<T>,
														   bool seedWithPrevious = false)
	{
		std::vector<dependency_pack> res;
		(std::invoke([&]() {
			 res.emplace_back(dependency_pack(
				 psl::templates::type_container<typename std::tuple_element<Is, T>::type>{}, seedWithPrevious));
		 }),
		 ...);
		return res;
	}


	template <std::size_t... Is, typename... Ts>
	std::tuple<Ts...> compress_from_dependency_pack(std::index_sequence<Is...>,
													psl::templates::type_container<std::tuple<Ts...>>,
													std::vector<dependency_pack>& pack)
	{
		return std::tuple<Ts...>{pack[Is].to_pack(
			psl::templates::type_container<
				typename std::remove_reference<decltype(std::get<Is>(std::declval<std::tuple<Ts...>>()))>::type>{})...};
	}

	class system_information final
	{
	  public:
		using pack_generator_type   = std::function<std::vector<details::dependency_pack>(bool)>;
		using system_invocable_type = std::function<void(psl::ecs::info&, std::vector<details::dependency_pack>)>;
		system_information()		= default;
		system_information(psl::ecs::threading threading, pack_generator_type&& generator,
						   system_invocable_type&& invocable, bool seedWithExisting = false)
			: m_Threading(threading), m_PackGenerator(std::move(generator)), m_System(std::move(invocable)),
			  m_SeedWithExisting(seedWithExisting){};
		~system_information()						  = default;
		system_information(const system_information&) = default;
		system_information(system_information&&)	  = default;
		system_information& operator=(const system_information&) = default;
		system_information& operator=(system_information&&) = default;

		std::vector<details::dependency_pack> create_pack() { return std::invoke(m_PackGenerator, m_SeedWithExisting); }

		void operator()(psl::ecs::info& info, std::vector<details::dependency_pack> packs)
		{
			m_SeedWithExisting = false;
			std::invoke(m_System, info, packs);
		}

		system_invocable_type& system() { return m_System; };

		psl::ecs::threading threading() const noexcept { return m_Threading; };

	  private:
		psl::ecs::threading m_Threading = threading::sequential;
		pack_generator_type m_PackGenerator;
		system_invocable_type m_System;
		bool m_SeedWithExisting{false};
	};
} // namespace psl::ecs::details