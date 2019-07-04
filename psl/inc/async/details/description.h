#pragma once
#include "../barrier.h"
#include "task.h"
#include "unique_ptr.h"
#include "array.h"
#include "array_view.h"
namespace psl::async
{
	class scheduler;
}
namespace psl::async::details
	{
	struct description
	{
		friend class ::psl::async::scheduler;

	  public:
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

		void barriers(barrier barrier) { m_Barriers.emplace_back(barrier); }
		void barriers(const psl::array<barrier>& barriers)
		{
			m_Barriers.insert(std::end(m_Barriers), std::begin(barriers), std::end(barriers));
		}

		void dynamic_barriers(std::future<barrier>&& barrier) { m_DynamicBarriers.emplace_back(std::move(barrier)); }
		void dynamic_barriers(std::shared_future<barrier>& barrier) { m_SharedDynamicBarriers.emplace_back(barrier); }
		void dynamic_barriers(psl::array<std::future<barrier>>&& barriers)
		{
			m_DynamicBarriers.insert(std::end(m_DynamicBarriers), std::make_move_iterator(std::begin(barriers)), std::make_move_iterator(std::end(barriers)));
			barriers.clear();
		}
		void dynamic_barriers(const psl::array<std::shared_future<barrier>>& barriers)
		{
			m_SharedDynamicBarriers.insert(std::end(m_SharedDynamicBarriers), std::begin(barriers), std::end(barriers));
		}

		void blocking(token token) { m_Blocking.emplace_back(token); }
		void blocking(const psl::array<token>& tokens)
		{
			m_Blocking.insert(std::end(m_Blocking), std::begin(tokens), std::end(tokens));
		}

	  private:
		psl::array_view<size_t> blockers() const noexcept { return m_Blockers; }
		psl::array_view<barrier> barriers() const noexcept { return m_Barriers; }
		psl::array_view<size_t> blocking() const noexcept { return m_Blocking; }

		bool dynamic_barriers_ready() const noexcept
		{
			return std::all_of(std::begin(m_DynamicBarriers), std::end(m_DynamicBarriers),
							   [](const std::future<barrier>& barrier) { return barrier.valid(); }) &&
				   std::all_of(std::begin(m_SharedDynamicBarriers), std::end(m_SharedDynamicBarriers),
							   [](const std::shared_future<barrier>& barrier) { return barrier.valid(); });
		}

		void merge_dynamic_barriers()
		{
			std::transform(std::begin(m_DynamicBarriers), std::end(m_DynamicBarriers), std::back_inserter(m_Barriers),
						   [](std::future<barrier>& barrier) {
							   assert_debug_break(barrier.valid());
							   return barrier.get();
						   });
			m_DynamicBarriers.clear();

			std::transform(std::begin(m_SharedDynamicBarriers), std::end(m_SharedDynamicBarriers),
						   std::back_inserter(m_Barriers), [](std::shared_future<barrier>& barrier) {
							   assert_debug_break(barrier.valid());
							   return barrier.get();
						   });
			m_SharedDynamicBarriers.clear();
		}

		psl::array<barrier> m_Barriers{};								   // what are my memory constraints
		psl::array<std::future<barrier>> m_DynamicBarriers{};			   // what are my dynamic memory constraints
		psl::array<std::shared_future<barrier>> m_SharedDynamicBarriers{}; // what are my dynamic memory constraints
		psl::array<size_t> m_Blocking{};								   // who am I blocking
		psl::array<size_t> m_Blockers{};								   // who blocks me
	};
} // namespace psl::async::details