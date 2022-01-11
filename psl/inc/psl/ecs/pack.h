#pragma once
#include "details/selectors.h"
#include "entity.h"
#include "selectors.h"


namespace psl::ecs
{
	class state_t;

	/// \brief an iterable container to work with components and entities.
	template <typename... Ts>
	class pack
	{
		friend class psl::ecs::state_t;

		template <typename... Ys>
		struct type_pack
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
			using policy_t		= psl::ecs::full;
		};

		template <typename... Ys>
		struct type_pack<psl::ecs::full, Ys...>
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
			using policy_t		= psl::ecs::full;
		};

		template <typename... Ys>
		struct type_pack<psl::ecs::partial, Ys...>
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
			using policy_t		= psl::ecs::partial;
		};


		template <typename T, typename... Ys>
		void check_policy()
		{
			static_assert(!psl::ecs::details::has_type<psl::ecs::partial, std::tuple<Ys...>>::value &&
							!psl::ecs::details::has_type<psl::ecs::partial, std::tuple<Ys...>>::value,
						  "policy types such as 'partial' and 'full' can only appear as the first type");
		}

	  public:
		static constexpr bool has_entities {std::disjunction<std::is_same<psl::ecs::entity, Ts>...>::value};

		using pack_t		= typename type_pack<Ts...>::pack_t;
		using filter_t		= typename type_pack<Ts...>::filter_t;
		using combine_t		= typename type_pack<Ts...>::combine_t;
		using break_t		= typename type_pack<Ts...>::break_t;
		using add_t			= typename type_pack<Ts...>::add_t;
		using remove_t		= typename type_pack<Ts...>::remove_t;
		using except_t		= typename type_pack<Ts...>::except_t;
		using conditional_t = typename type_pack<Ts...>::conditional_t;
		using order_by_t	= typename type_pack<Ts...>::order_by_t;
		using policy_t		= typename type_pack<Ts...>::policy_t;

		static_assert(std::tuple_size<order_by_t>::value <= 1, "multiple order_by statements make no sense");

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

		auto begin() const noexcept -> typename pack_t::unpack_iterator { return m_Pack.unpack_begin(); }

		auto end() const noexcept -> typename pack_t::unpack_iterator { return m_Pack.unpack_end(); }
		constexpr size_t size() const noexcept { return m_Pack.size(); }

	  private:
		pack_t m_Pack;
	};
}	 // namespace psl::ecs