#pragma once
#include <atomic>
#include "unique_ptr.h"
#include "view_ptr.h"
#include "description.h"
#include "task.h"
#include "../token.h"

namespace psl::async::details
{
	struct packet
	{
	  public:
		packet(token token, psl::unique_ptr<details::task_base>&& task) noexcept
			: m_Token(token), m_Task(std::move(task))
		{
			m_Done.store(false, std::memory_order_relaxed);
		}

		packet(token token) noexcept : m_Token(token), m_Task(nullptr)
		{
			m_Done.store(false, std::memory_order_relaxed);
		}

		~packet() = default;

		packet(const packet& other) = delete;
		packet& operator=(const packet& other) = delete;
		packet(packet&& other) noexcept
			: m_Description(std::move(other.m_Description)), m_Dependencies(std::move(other.m_Dependencies)),
			  m_Token(other.m_Token), m_Task(std::move(other.m_Task)), m_Heuristic(other.m_Heuristic),
			  m_Done(other.m_Done.load(std::memory_order_acquire)){};
		packet& operator=(packet&& other) noexcept
		{
			if(this != &other)
			{
				m_Description  = std::move(other.m_Description);
				m_Dependencies = std::move(other.m_Dependencies);
				m_Token		   = other.m_Token;
				m_Task		   = std::move(other.m_Task);
				m_Heuristic	= other.m_Heuristic;
				m_Done.store(other.m_Done.load(std::memory_order_acquire), std::memory_order_relaxed);
			}
			return *this;
		};

		operator size_t() const noexcept { return m_Token.operator size_t(); }

		/// \warning Only invoke in owning thread
		void operator()()
		{
			if(is_ready()) return;
			(*m_Task)();
			m_Done.store(true, std::memory_order_relaxed);
		}

		void reset() { m_Done.store(false, std::memory_order_relaxed); }

		/// \brief Multithread safe way of accessing the current state of the packet
		bool is_ready() const noexcept { return m_Done.load(std::memory_order_relaxed); }

		bool has_task() const noexcept { return m_Task; };

		const details::description& description() const noexcept { return m_Description; }
		details::description& description() noexcept { return m_Description; }

		void substitute(psl::unique_ptr<details::task_base>&& task) { m_Task = std::move(task); }

	  private:
		details::description m_Description{};
		psl::array<psl::view_ptr<packet>> m_Dependencies{}; // things I depend on
		token m_Token;
		psl::unique_ptr<details::task_base> m_Task;
		int64_t m_Heuristic{0};
		std::atomic<bool> m_Done;
	};
} // namespace psl::async::details