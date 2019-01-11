#pragma once
#include "stdafx.h"
#include "ecs/entity.h"
#include "ecs/range.h"
#include "ecs/selectors.h"


namespace core::ecs
{
	/// \brief an iterable container to work with components and entities.
	template <typename... Ts>
	class pack
	{
		friend class core::ecs::state;

	  public:
		static constexpr bool has_entities{std::disjunction<std::is_same<core::ecs::entity, Ts>...>::value};

		using pack_t	= typename details::typelist_to_pack_view<Ts...>::type;
		using filter_t  = typename details::typelist_to_pack<Ts...>::type;
		using combine_t = typename details::typelist_to_combine_pack<Ts...>::type;
		using break_t   = typename details::typelist_to_break_pack<Ts...>::type;
		using add_t		= typename details::typelist_to_add_pack<Ts...>::type;
		using remove_t  = typename details::typelist_to_remove_pack<Ts...>::type;
		using except_t  = typename details::typelist_to_except_pack<Ts...>::type;

	  public:
		pack() : m_Pack(){};

		pack(pack_t views) : m_Pack(views) {}

		pack_t view() { return m_Pack; }

		template <typename T>
		psl::array_view<T> get() const noexcept
		{
			return m_Pack.template get<T>();
		}

		template <size_t N>
		auto get() const noexcept
		{
			return m_Pack.get<N>();
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
		auto reference_get() noexcept 
		{
			return m_Pack.ref_get<N>();
		}

		template <typename T>
		psl::array_view<T>& reference_get() noexcept
		{
			return m_Pack.template ref_get<T>();
		}

		pack_t m_Pack;
	};
} // namespace core::ecs