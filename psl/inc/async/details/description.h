#pragma once
#include "../barrier.h"
#include "task.h"
#include "unique_ptr.h"
#include "array.h"
#include "array_view.h"

namespace psl::async2::details
{
	struct description
	{
		description()					= default;
		description(const description&) = delete;
		description& operator=(const description&) = delete;
		description(description&& other) noexcept
			: m_Barriers(std::move(other.m_Barriers)), m_Blocking(std::move(other.m_Blocking))
		{}

		description& operator=(description&& other) noexcept
		{
			if(this != &other)
			{
				m_Barriers = std::move(other.m_Barriers);
				m_Blocking = std::move(other.m_Blocking);
			}
			return *this;
		}

		void blockers(token token) { m_Blockers.emplace_back(token); }
		void blockers(const psl::array<token>& tokens)
		{
			m_Blockers.insert(std::end(m_Blockers), std::begin(tokens), std::end(tokens));
		}
		psl::array_view<size_t> blockers() const noexcept { return m_Blockers; }

		void barriers(barrier barrier) { m_Barriers.emplace_back(barrier); }
		void barriers(const psl::array<barrier>& barriers)
		{
			m_Barriers.insert(std::end(m_Barriers), std::begin(barriers), std::end(barriers));
		}
		psl::array_view<barrier> barriers() const noexcept { return m_Barriers; }

		void blocking(token token) { m_Blocking.emplace_back(token); }
		void blocking(const psl::array<token>& tokens)
		{
			m_Blocking.insert(std::end(m_Blocking), std::begin(tokens), std::end(tokens));
		}
		psl::array_view<size_t> blocking() const noexcept { return m_Blocking; }

		psl::array<barrier> m_Barriers{}; // what are my memory constraints
		psl::array<size_t> m_Blocking{};  // who am I blocking
		psl::array<size_t> m_Blockers{};  // who blocks me
	};
} // namespace psl::async2::details