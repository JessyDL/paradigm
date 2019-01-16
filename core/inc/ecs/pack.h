#pragma once
#include "ecs/entity.h"
#include "ecs/range.h"
#include "ecs/selectors.h"


namespace core::ecs
{
	class state;

	struct partial
	{};
	struct full
	{};

	/// \brief an iterable container to work with components and entities.
	template <typename... Ts>
	class pack
	{
		friend class core::ecs::state;

		template <typename... Ys>
		struct type_pack
		{
			using pack_t	= typename details::typelist_to_pack_view<Ys...>::type;
			using filter_t  = typename details::typelist_to_pack<Ys...>::type;
			using combine_t = typename details::typelist_to_combine_pack<Ys...>::type;
			using break_t   = typename details::typelist_to_break_pack<Ys...>::type;
			using add_t		= typename details::typelist_to_add_pack<Ys...>::type;
			using remove_t  = typename details::typelist_to_remove_pack<Ys...>::type;
			using except_t  = typename details::typelist_to_except_pack<Ys...>::type;
			using policy_t  = core::ecs::full;
		};

		template <typename... Ys>
		struct type_pack<core::ecs::full, Ys...>
		{
			using pack_t	= typename details::typelist_to_pack_view<Ys...>::type;
			using filter_t  = typename details::typelist_to_pack<Ys...>::type;
			using combine_t = typename details::typelist_to_combine_pack<Ys...>::type;
			using break_t   = typename details::typelist_to_break_pack<Ys...>::type;
			using add_t		= typename details::typelist_to_add_pack<Ys...>::type;
			using remove_t  = typename details::typelist_to_remove_pack<Ys...>::type;
			using except_t  = typename details::typelist_to_except_pack<Ys...>::type;
			using policy_t  = core::ecs::full;
		};

		template <typename... Ys>
		struct type_pack<core::ecs::partial, Ys...>
		{
			using pack_t	= typename details::typelist_to_pack_view<Ys...>::type;
			using filter_t  = typename details::typelist_to_pack<Ys...>::type;
			using combine_t = typename details::typelist_to_combine_pack<Ys...>::type;
			using break_t   = typename details::typelist_to_break_pack<Ys...>::type;
			using add_t		= typename details::typelist_to_add_pack<Ys...>::type;
			using remove_t  = typename details::typelist_to_remove_pack<Ys...>::type;
			using except_t  = typename details::typelist_to_except_pack<Ys...>::type;
			using policy_t  = core::ecs::partial;
		};


		template <typename T, typename... Ys>
		void check_policy()
		{
			static_assert(!core::ecs::details::has_type<core::ecs::partial, std::tuple<Ys...>>::value &&
							  !core::ecs::details::has_type<core::ecs::partial, std::tuple<Ys...>>::value,
						  "policy types such as 'partial' and 'full' can only appear as the first type");
		}

	  public:
		static constexpr bool has_entities{std::disjunction<std::is_same<core::ecs::entity, Ts>...>::value};

		using pack_t	= typename type_pack<Ts...>::pack_t;
		using filter_t  = typename type_pack<Ts...>::filter_t;
		using combine_t = typename type_pack<Ts...>::combine_t;
		using break_t   = typename type_pack<Ts...>::break_t;
		using add_t		= typename type_pack<Ts...>::add_t;
		using remove_t  = typename type_pack<Ts...>::remove_t;
		using except_t  = typename type_pack<Ts...>::except_t;
		using policy_t  = typename type_pack<Ts...>::policy_t;

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

		auto operator[](size_t index) const noexcept -> decltype(std::declval<typename pack_t::iterator>().operator*())
		{
			return m_Pack[index];
		}

		auto begin() const noexcept -> typename pack_t::iterator { return std::begin(m_Pack); }

		auto end() const noexcept -> typename pack_t::iterator { return std::end(m_Pack); }
		constexpr size_t size() const noexcept { return m_Pack.size(); }

	  private:
		// todo: we should elminate the need for this.
		template <size_t N>
		auto reference_get() noexcept -> decltype(std::declval<pack_t>().template ref_get<N>())
		{
			return m_Pack.template ref_get<N>();
		}

		template <typename T>
		psl::array_view<T>& reference_get() noexcept
		{
			return m_Pack.template ref_get<T>();
		}

		pack_t m_Pack;
	};
} // namespace core::ecs