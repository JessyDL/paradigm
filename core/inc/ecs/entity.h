#pragma once
#include <cstdint>
#include <functional>
namespace core::ecs
{
	/// ----------------------------------------------------------------------------------------------
	/// Entity
	/// ----------------------------------------------------------------------------------------------
	struct entity;
	static bool valid(entity e) noexcept;

	/// \brief entity points to a collection of components
	struct entity
	{
	public:
		using value_type = uint64_t;
		entity() = default;
		entity(value_type id) : m_ID(id) {};
		~entity() = default;
		entity(const entity&) = default;
		entity& operator=(const entity&) = default;
		entity(entity&&) = default;
		entity& operator=(entity&&) = default;

		bool valid() const noexcept
		{
			return m_ID != 0u;
		}
		friend bool valid(entity e) noexcept;
		bool operator==(entity b) const noexcept
		{
			return m_ID == b.m_ID;
		}
		bool operator<=(entity b) const noexcept
		{
			return m_ID <= b.m_ID;
		}
		bool operator!=(entity b) const noexcept
		{
			return m_ID != b.m_ID;
		}
		bool operator>=(entity b) const noexcept
		{
			return m_ID >= b.m_ID;
		}
		bool operator<(entity b) const noexcept
		{
			return m_ID < b.m_ID;
		}
		bool operator>(entity b) const noexcept
		{
			return m_ID > b.m_ID;
		}

		value_type id() const noexcept { return m_ID; };
	private:
		value_type m_ID;
	};

	/// \brief checks if an entity is valid or not
	/// \param[in] e the entity to check
	static bool valid(entity e) noexcept
	{
		return e.m_ID != 0u;
	};
}

namespace std
{
	template <>
	struct hash<core::ecs::entity>
	{
		std::size_t operator()(core::ecs::entity const& e) const noexcept
		{
			return e.id();
		}
	};
} // namespace std