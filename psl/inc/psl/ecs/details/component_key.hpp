#pragma once
#include "../selectors.hpp"
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "strtype/strtype.hpp"

namespace psl::ecs::details
{
	class component_key_t;
}
namespace std
{
	template <>
	struct hash<psl::ecs::details::component_key_t>;
}

namespace psl::ecs::details
{
	class component_key_t
	{
		friend struct std::hash<component_key_t>;

		consteval std::uint32_t fnv1a_32(std::string_view value) const noexcept
		{
			std::uint32_t seed {2166136261u};
			for(auto c : value)
			{
				seed ^= c * 16777619u;
			}
			return seed;
		}

	  public:
		consteval component_key_t(std::string_view name) noexcept : m_Name(name), m_Value(fnv1a_32(name)) {}

		constexpr component_key_t(const component_key_t& other) noexcept : m_Name(other.m_Name), m_Value(other.m_Value)
		{}
		constexpr component_key_t& operator=(const component_key_t& other) noexcept
		{
			if(this != &other)
			{
				m_Name	= other.m_Name;
				m_Value = other.m_Value;
			}
			return *this;
		}
		constexpr component_key_t(component_key_t&& other) noexcept : m_Name(other.m_Name), m_Value(other.m_Value) {};
		constexpr component_key_t& operator=(component_key_t&& other) noexcept
		{
			if(this != &other)
			{
				m_Name	= other.m_Name;
				m_Value = other.m_Value;
			}
			return *this;
		}

		inline constexpr auto name() const noexcept { return m_Name; }
		inline constexpr operator std::string_view() const noexcept { return m_Name; }
		inline constexpr auto operator==(const component_key_t& other) const noexcept -> bool
		{
			return m_Value == other.m_Value && m_Name == other.m_Name;
		}
		inline constexpr auto operator!=(const component_key_t& other) const noexcept -> bool
		{
			return m_Value != other.m_Value || m_Name != other.m_Name;
		}

		inline constexpr auto operator<(const component_key_t& other) const noexcept -> bool
		{
			return m_Value < other.m_Value;
		}
		inline constexpr auto operator>(const component_key_t& other) const noexcept -> bool
		{
			return m_Value > other.m_Value;
		}

	  private:
		std::string_view m_Name;
		std::uint32_t m_Value;
	};

	template <typename T, auto TName = strtype::stringify_typename<std::remove_pointer_t<std::remove_cvref_t<T>>>()>
	constexpr auto key_for() noexcept -> component_key_t
	{
		constexpr component_key_t result {TName};
		return result;
	}
}	 // namespace psl::ecs::details

namespace std
{
	template <>
	struct hash<psl::ecs::details::component_key_t>
	{
		std::size_t operator()(const psl::ecs::details::component_key_t& k) const
		{
			using psl::ecs::details::component_key_t;
			return std::hash<decltype(component_key_t::m_Value)> {}(k.m_Value);
		}
	};
}	 // namespace std
