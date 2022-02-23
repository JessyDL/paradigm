#pragma once
#include "details/selectors.hpp"
#include "entity.hpp"
#include "selectors.hpp"


namespace psl::ecs
{
	class state_t;

	namespace
	{
		template <typename... Ys>
		struct pack_base_types
		{
			using pack_t		= typename details::typelist_to_pack_view<Ys...>::type;
			using filter_t		= typename details::typelist_to_pack<Ys...>::type;
			using combine_t		= typename details::typelist_to_combine_pack<Ys...>::type;
			using break_t		= typename details::typelist_to_break_pack<Ys...>::type;
			using add_t			= typename details::typelist_to_add_pack<Ys...>::type;
			using remove_t		= typename details::typelist_to_remove_pack<Ys...>::type;
			using except_t		= typename details::typelist_to_except_pack<Ys...>::type;
			using conditional_t = typename details::typelist_to_conditional_pack<Ys...>::type;
			using order_by_t	= typename details::typelist_to_orderby_pack<Ys...>::type;
		};

		template <typename... Ys>
		struct pack_base : public pack_base_types<Ys...>
		{
			using policy_t = psl::ecs::full;
		};

		template <typename... Ys>
		struct pack_base<psl::ecs::full, Ys...> : public pack_base_types<Ys...>
		{
			using policy_t = psl::ecs::full;
		};

		template <typename... Ys>
		struct pack_base<psl::ecs::partial, Ys...> : public pack_base_types<Ys...>
		{
			using policy_t = psl::ecs::partial;
		};
	}

	/// \brief an iterable container to work with components and entities.
	template <typename... Ts>
	class pack : public pack_base<Ts...>
	{
		template <typename T, typename... Ys>
		void check_policy()
		{
			static_assert(!psl::HasType<psl::ecs::partial, psl::type_pack_t<Ys...>> &&
							!psl::HasType<psl::ecs::full, psl::type_pack_t<Ys...>>,
						  "policy types such as 'partial' and 'full' can only appear as the first type");
		}

	  public:
		static constexpr bool has_entities {std::disjunction<std::is_same<psl::ecs::entity, Ts>...>::value};
		
		using pack_t        = typename pack_base<Ts...>::pack_t;
		using filter_t      = typename pack_base<Ts...>::filter_t;
		using combine_t     = typename pack_base<Ts...>::combine_t;
		using break_t       = typename pack_base<Ts...>::break_t;
		using add_t         = typename pack_base<Ts...>::add_t;
		using remove_t      = typename pack_base<Ts...>::remove_t;
		using except_t      = typename pack_base<Ts...>::except_t;
		using conditional_t = typename pack_base<Ts...>::conditional_t;
		using order_by_t    = typename pack_base<Ts...>::order_by_t;
		using policy_t      = typename pack_base<Ts...>::policy_t;

		static_assert(std::tuple_size<order_by_t>::value <= 1,
					  "multiple order_by statements make no sense");

	  public:
		pack() : m_Pack() { check_policy<Ts...>(); };
		pack(pack_t views) : m_Pack(views) { check_policy<Ts...>(); }
		pack_t view() { return m_Pack; }

		template <typename T>
		psl::array_view<T> get() const noexcept
		{
			return m_Pack.template get<T>();
		}

		template <size_t N>
		auto get() const noexcept -> decltype(std::declval<pack_t>().template get<N>())
		{
			return m_Pack.template get<N>();
		}

		auto operator[](size_t index) const noexcept { return m_Pack.unpack(index); }
		auto operator[](size_t index) noexcept { return m_Pack.unpack(index); }
		auto begin() const noexcept { return m_Pack.unpack_begin(); }
		auto end() const noexcept { return m_Pack.unpack_end(); }
		constexpr auto size() const noexcept -> size_t { return m_Pack.size(); }
		constexpr auto empty() const noexcept -> bool { return m_Pack.size() == 0; }

	  private:
		pack_t m_Pack;
	};
}	 // namespace psl::ecs