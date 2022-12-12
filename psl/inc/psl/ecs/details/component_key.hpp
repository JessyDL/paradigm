#pragma once
#include "../selectors.hpp"
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "psl/ecs/component_name.hpp"
#include "psl/ecs/component_traits.hpp"
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
	/// \brief Container type for unique component types. These should be construced using the `component_key_t::generate` helper function.
	/// \details Internally contains the component type's stringified name, and the hash.
	/// Both are generated at compile time using the `psl::ecs::details::component_key_t::generate` helper function.
	class component_key_t
	{
		friend struct std::hash<component_key_t>;

		template <typename T>
		struct type_container
		{
			static constexpr auto name = strtype::stringify_typename<T>();
		};

		template <details::HasComponentNameOverride T>
		struct type_container<T>
		{
			static constexpr auto name = T::_ECS_COMPONENT_NAME;
		};

		constexpr std::uint32_t fnv1a_32(std::string_view value) const noexcept
		{
			std::uint32_t seed {2166136261u};
			for(auto c : value)
			{
				seed ^= c * 16777619u;
			}
			return seed;
		}

		template <typename T>
		consteval component_key_t(const type_container<T>& name) noexcept :
			m_Name(type_container<T>::name), m_Value(fnv1a_32(type_container<T>::name)),
			m_Type(std::is_empty_v<T>	  ? component_type::FLAG
				   : std::is_trivial_v<T> ? component_type::TRIVIAL
										  : component_type::COMPLEX),
			m_StringMemory(nullptr)
		{}

	  public:
		constexpr component_key_t(std::string_view name, component_type type) :
			m_Name(name), m_Value(fnv1a_32(name)), m_Type(type), m_StringMemory(nullptr)
		{
			if(!std::is_constant_evaluated())
			{
				m_StringMemory = (char*)malloc(sizeof(char) * name.size());
				memcpy(m_StringMemory, name.data(), sizeof(char) * name.size());
				m_Name = std::string_view(m_StringMemory, name.size());
			}
		}

		constexpr ~component_key_t()
		{
			if(!std::is_constant_evaluated() && m_StringMemory)
			{
				free(m_StringMemory);
			}
		}

		constexpr component_key_t(const component_key_t& other) :
			m_Name(other.m_Name), m_Value(other.m_Value), m_Type(other.m_Type), m_StringMemory(nullptr)
		{
			if(std::find(std::begin(m_Name), std::end(m_Name), '<') != std::end(m_Name))
			{
				throw std::runtime_error("templated component types are not supported (due to portability issues)");
			}

			if(!std::is_constant_evaluated() && other.m_StringMemory)
			{
				m_StringMemory = (char*)malloc(sizeof(char) * other.m_Name.size());
				memcpy(m_StringMemory, other.m_Name.data(), sizeof(char) * other.m_Name.size());
				m_Name = std::string_view(m_StringMemory, other.m_Name.size());
			}
		}
		constexpr component_key_t& operator=(const component_key_t& other) noexcept
		{
			if(this != &other)
			{
				m_Name		   = other.m_Name;
				m_Value		   = other.m_Value;
				m_Type		   = other.m_Type;
				m_StringMemory = nullptr;

				if(!std::is_constant_evaluated() && other.m_StringMemory)
				{
					m_StringMemory = (char*)malloc(sizeof(char) * other.m_Name.size());
					memcpy(m_StringMemory, other.m_Name.data(), sizeof(char) * other.m_Name.size());
					m_Name = std::string_view(m_StringMemory, other.m_Name.size());
				}
			}
			return *this;
		}
		constexpr component_key_t(component_key_t&& other) noexcept :
			m_Name(other.m_Name), m_Value(other.m_Value), m_Type(other.m_Type), m_StringMemory(other.m_StringMemory)
		{
			other.m_StringMemory = nullptr;
		};
		constexpr component_key_t& operator=(component_key_t&& other) noexcept
		{
			if(this != &other)
			{
				m_Name				 = other.m_Name;
				m_Value				 = other.m_Value;
				m_Type				 = other.m_Type;
				m_StringMemory		 = other.m_StringMemory;
				other.m_StringMemory = nullptr;
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

		/// \brief Generates a `component_key_t` based on the given type in a cross platform safe manner.
		/// \note Strips const, volatile, reference, and pointer designations of the template type.
		/// \warning watch out with modifying this issue, see: https://developercommunity.visualstudio.com/t/constexpr-unable-to-call-private-constructor-in-st/82639
		template <typename T>
		static constexpr auto generate() noexcept -> component_key_t
		{
			return component_key_t {type_container<std::remove_pointer_t<std::remove_cvref_t<T>>> {}};
		}

		component_type type() const noexcept { return m_Type; }

	  private:
		std::string_view m_Name;
		std::uint32_t m_Value;
		component_type m_Type;
		char* m_StringMemory;
	};
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
