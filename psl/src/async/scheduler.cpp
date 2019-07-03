#include "async/scheduler.h"
#include "view_ptr.h"
#include "task.h"

using namespace psl::async2;


scheduler::scheduler(std::optional<size_t> workers) noexcept
	: m_Workers(workers.value_or(std::thread::hardware_concurrency()))
{}


void scheduler::execute()
{
	struct worker
	{
	  public:
		worker(psl::spmc::consumer<psl::view_ptr<details::packet>>&& consumer) : m_Consumer(std::move(consumer)){};
		~worker()
		{
			if(!terminated()) terminate();
			while(!m_Thread.joinable())
			{
			};
			m_Thread.join();
		}
		worker(const worker&)	 = default;
		worker(worker&&) noexcept = default;
		worker& operator=(const worker&) = default;
		worker& operator=(worker&&) noexcept = default;
		void start()
		{
			m_Done.store(false, std::memory_order_relaxed);
			m_Thread = std::thread{&worker::loop, this};
		}
		void terminate() { m_Run.store(false, std::memory_order_relaxed); }
		bool terminated() { return m_Done.load(std::memory_order_relaxed); }

	  private:
		void loop()
		{
			while(m_Run.load(std::memory_order_relaxed))
			{
				if(auto item = m_Consumer.pop(); item)
				{
					auto task = item.value();
					task->operator()();
				}
			}
			m_Done.store(true, std::memory_order_relaxed);
		}

		std::thread m_Thread{};
		psl::spmc::consumer<psl::view_ptr<details::packet>> m_Consumer;
		std::atomic<bool> m_Run{true};
		std::atomic<bool> m_Done{false};
	};

	struct invocable_comparer
	{
		bool operator()(psl::view_ptr<details::packet> lhs, size_t rhs) const { return *lhs < rhs; }

		bool operator()(size_t lhs, psl::view_ptr<details::packet> rhs) const { return lhs < *rhs; }
	};


	psl::array<barrier> write_barriers{};
	psl::array<barrier> read_barriers{};
	psl::array<size_t> done{};

	psl::spmc::producer<psl::view_ptr<details::packet>> tasks;
	psl::array<psl::view_ptr<details::packet>> inflight;
	psl::array<psl::view_ptr<details::packet>> invocables;

	std::transform(std::begin(m_Invocables), std::end(m_Invocables), std::back_inserter(invocables),
				   [](auto& packet) { return psl::view_ptr<details::packet>(&packet); });

	for(auto packet : invocables)
	{
		if(packet->description().blockers().size() == 0)
		{
			tasks.push(psl::view_ptr<details::packet>(packet));
			inflight.emplace_back(packet);
		}
	}

	{
		auto inv_copy = invocables;
		invocables.clear();
		std::set_difference(std::begin(inv_copy), std::end(inv_copy), std::begin(inflight), std::end(inflight),
							std::back_inserter(invocables));
	}

	psl::array<psl::unique_ptr<worker>> workers;
	auto worker_count = std::min<size_t>(m_Workers - 1u, 1u);
	workers.reserve(worker_count);
	for(auto i = 0; i < worker_count; ++i)
	{
		workers.emplace_back(new worker(tasks.consumer()));
		workers[i]->start();
	}

	while(inflight.size() > 0 || invocables.size() > 0)
	{
		if(auto item = tasks.pop(); item)
		{
			auto task = item.value();
			task->operator()();
		}

		if(auto it = std::remove_if(std::begin(inflight), std::end(inflight),
									[](const auto& packet) { return packet->is_ready(); });
		   it != std::end(inflight))
		{
			auto done_mid = done.size();
			std::transform(it, std::end(inflight), std::back_inserter(done),
						   [](const auto& task) -> size_t { return task->operator size_t(); });

			
			auto inv_copy = invocables;
			invocables.clear();
			std::set_difference(std::begin(inv_copy), std::end(inv_copy), std::next(std::begin(done), done_mid),
								std::end(done), std::back_inserter(invocables), invocable_comparer());

			std::inplace_merge(std::begin(done), std::next(std::begin(done), done_mid), std::end(done));
			inflight.erase(it, std::end(inflight));


			for(auto& packet : invocables)
			{
				if(!packet->is_ready() &&
				   std::includes(std::begin(done), std::end(done), std::begin(packet->description().blockers()),
								 std::end(packet->description().blockers())))
				{
					tasks.push(psl::view_ptr<details::packet>(packet));
					inflight.emplace_back(packet);
				}
			}
		}
	}

	for(auto i = 0; i < worker_count; ++i)
	{
		workers[i]->terminate();
	}
	m_TokenOffset += m_Invocables.size();
}

auto description_sort = [](const details::description& lhs, const details::description& rhs) -> bool {
	return lhs.barriers().size() + lhs.blocking().size() <= rhs.barriers().size() + rhs.blocking().size();
};

void scheduler::sequence(token first, token then) noexcept
{
	m_Invocables[then - m_TokenOffset].description().blockers(first);
}
void scheduler::sequence(psl::array<token> first, token then) noexcept
{
	m_Invocables[then - m_TokenOffset].description().blockers(first);
}
void scheduler::sequence(token first, psl::array<token> then) noexcept
{
	for(const auto& t : then) m_Invocables[t - m_TokenOffset].description().blockers(first);
}
void scheduler::sequence(psl::array<token> first, psl::array<token> then) noexcept
{
	for(const auto& t : then) m_Invocables[t - m_TokenOffset].description().blockers(first);
}

void scheduler::barriers(token token, psl::array<barrier> barriers) noexcept
{
	m_Invocables[token - m_TokenOffset].description().barriers(barriers);
}
void scheduler::consecutive(token target, psl::array<token> tokens) noexcept
{
	m_Invocables[target - m_TokenOffset].description().blocking(tokens);
}