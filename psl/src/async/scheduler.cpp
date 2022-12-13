#include "psl/async/scheduler.hpp"
#include "psl/collections/spmc/consumer.hpp"
#include "psl/view_ptr.hpp"

using namespace psl::async;

namespace psl::async::details
{
struct worker
{
  public:
	worker() = delete;
	worker(psl::spmc::consumer<psl::view_ptr<details::packet>>&& consumer) : m_Consumer(std::move(consumer)) {};
	~worker()
	{
		if(!terminated()) resume(), terminate();
		while(!m_Thread.joinable())
		{};
		m_Thread.join();
	}
	worker(const worker& other) : m_Consumer(other.m_Consumer) {};
	worker(worker&&)				 = delete;
	worker& operator=(const worker&) = delete;
	worker& operator=(worker&&)		 = delete;
	void start()
	{
		m_Done.store(false, std::memory_order_relaxed);
		m_Thread = std::thread {&worker::loop, this};
	}
	void terminate() { m_Run.store(false, std::memory_order_relaxed); }
	bool terminated() { return m_Done.load(std::memory_order_relaxed); }

	void pause()
	{
		if(m_Paused) return;
		// pause
		{
			std::lock_guard<std::mutex> lk(m);
			m_Paused = true;
		}
	}
	void resume()
	{
		if(!m_Paused) return;
		{
			std::lock_guard<std::mutex> lk(m);
			m_Paused = false;
		}
		cv.notify_one();
		// resume t2
	}

  private:
	void loop()
	{
		int max_spin = 1000;
		while(m_Run.load(std::memory_order_relaxed))
		{
			while(m_Paused)
			{
				std::unique_lock<std::mutex> lk(m);
				cv.wait(lk, [this]() { return !m_Paused; });
				lk.unlock();
			}

			if(auto item = m_Consumer.pop(); item)
			{
				auto task = item.value();
				task->operator()();
			}
			else
				--max_spin;

			if(max_spin == 0)
			{
				max_spin = 1000;
				std::this_thread::sleep_for(std::chrono::microseconds(5));
			}
		}
		m_Done.store(true, std::memory_order_relaxed);
	}

	std::thread m_Thread {};
	psl::spmc::consumer<psl::view_ptr<details::packet>> m_Consumer;
	std::atomic<bool> m_Run {true};
	std::atomic<bool> m_Done {false};


	bool m_Paused {true};
	std::condition_variable cv;
	std::mutex m;
};
}	 // namespace psl::async::details

scheduler::scheduler(std::optional<size_t> workers) noexcept :
	m_Workers(workers.value_or(std::thread::hardware_concurrency() - 1))
{
	m_Workerthreads.reserve(m_Workers);
	for(auto i = 0; i < m_Workers; ++i)
	{
		m_Workerthreads.emplace_back(new details::worker(m_Tasks.consumer()));
		m_Workerthreads[i]->start();
	}
}

scheduler::~scheduler() {}

void scheduler::execute()
{
	struct invocable_comparer
	{
		bool operator()(psl::view_ptr<details::packet> lhs, size_t rhs) const noexcept { return *lhs < rhs; }

		bool operator()(size_t lhs, psl::view_ptr<details::packet> rhs) const noexcept { return lhs < *rhs; }
	};

	auto compatible = [](psl::array_view<barrier> lhs, psl::array_view<barrier> rhs) {
		return std::all_of(std::begin(lhs), std::end(lhs), [&rhs](const barrier& lhs_barrier) {
			return std::none_of(std::begin(rhs), std::end(rhs), [&lhs_barrier](const barrier& rhs_barrier) {
				return ((lhs_barrier.type() == barrier_type::WRITE) || rhs_barrier.type() == barrier_type::WRITE) &&
					   lhs_barrier.overlaps(rhs_barrier);
			});
		});
	};

	// make sure no proxy objects exist
	// assert(std::none_of(std::begin(m_Invocables), std::end(m_Invocables), [](const auto& ptr) { return ptr ==
	// nullptr; }) == true);

	psl::array<barrier> barriers {};
	psl::array<size_t> done {};

	psl::array<psl::view_ptr<details::packet>> inflight;
	psl::array<psl::view_ptr<details::packet>> invocables;

	std::transform(std::begin(m_Invocables), std::end(m_Invocables), std::back_inserter(invocables), [](auto& packet) {
		return psl::view_ptr<details::packet>(&packet);
	});

	for(auto packet : invocables)
	{
		std::sort(std::begin(packet->description().m_Blockers), std::end(packet->description().m_Blockers));
		if(packet->description().blockers().size() == 0 && packet->description().dynamic_barriers_ready() &&
		   compatible(packet->description().barriers(), barriers))
		{
			m_Tasks.push(psl::view_ptr<details::packet>(packet));
			inflight.emplace_back(packet);
			barriers.insert(std::end(barriers),
							std::begin(packet->description().barriers()),
							std::end(packet->description().barriers()));
		}
	}

	{
		auto inv_copy = std::move(invocables);
		invocables.clear();
		std::set_difference(std::begin(inv_copy),
							std::end(inv_copy),
							std::begin(inflight),
							std::end(inflight),
							std::back_inserter(invocables));
	}
	for(auto& thread : m_Workerthreads) thread->resume();

	while(inflight.size() > 0)
	{
		psl_assert(std::unique(std::begin(inflight),
							   std::end(inflight),
							   [](const auto& lhs, const auto& rhs) { return *lhs == *rhs; }) == std::end(inflight),
				   "unique test failed");
		if(auto item = m_Tasks.pop(); item)
		{
			auto task = item.value();
			task->operator()();
		}

		if(auto it = std::stable_partition(std::begin(inflight),
										   std::end(inflight),
										   [](psl::view_ptr<details::packet> packet) { return !packet->is_ready(); });
		   it != std::end(inflight))
		{
			// Add all ready inflight tasks to the done list, and sort the result. Then remove them from the infight.
			auto done_mid = done.size();
			std::transform(it, std::end(inflight), std::back_inserter(done), [](const auto& task) -> size_t {
				return task->operator size_t();
			});

			std::inplace_merge(std::begin(done), std::next(std::begin(done), done_mid), std::end(done));

			inflight.erase(it, std::end(inflight));

			psl_assert(std::unique(std::begin(done), std::end(done)) == std::end(done), "unique test failed");

			// Redo the barriers
			barriers.clear();
			for(auto& packet : inflight)
			{
				barriers.insert(std::end(barriers),
								std::begin(packet->description().barriers()),
								std::end(packet->description().barriers()));
			}

			// Find new tasks to push into flight, and sort the result
			auto inflight_mid = inflight.size();
			for(auto& packet : invocables)
			{
				if(packet->description().dynamic_barriers_ready())
					packet->description().merge_dynamic_barriers();
				else
					continue;

				if(std::includes(std::begin(done),
								 std::end(done),
								 std::begin(packet->description().blockers()),
								 std::end(packet->description().blockers())) &&
				   compatible(packet->description().barriers(), barriers))
				{
					m_Tasks.push(psl::view_ptr<details::packet>(packet));
					inflight.emplace_back(packet);

					barriers.insert(std::end(barriers),
									std::begin(packet->description().barriers()),
									std::end(packet->description().barriers()));
				}
			}

			auto inv_copy = std::move(invocables);
			invocables.clear();
			std::set_difference(std::begin(inv_copy),
								std::end(inv_copy),
								std::next(std::begin(inflight), inflight_mid),
								std::end(inflight),
								std::back_inserter(invocables));

			std::inplace_merge(std::begin(inflight),
							   std::next(std::begin(inflight), inflight_mid),
							   std::end(inflight),
							   [](const auto& lhs, const auto& rhs) { return *lhs < *rhs; });
		}
	}
	for(auto& thread : m_Workerthreads) thread->pause();

	psl_assert(
	  m_Invocables.size() == done.size(), "there were still {} tasks in flight", m_Invocables.size() - done.size());
	m_TokenOffset += m_Invocables.size();
	m_Invocables.clear();
}

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

void scheduler::barriers(token token, const psl::array<barrier>& barriers)
{
	m_Invocables[token - m_TokenOffset].description().barriers(barriers);
}
void scheduler::barriers(token token, psl::array<std::future<barrier>>&& barriers)
{
	m_Invocables[token - m_TokenOffset].description().dynamic_barriers(std::move(barriers));
}
void scheduler::barriers(token token, std::future<barrier>&& barrier)
{
	m_Invocables[token - m_TokenOffset].description().dynamic_barriers(std::move(barrier));
}
void scheduler::barriers(token token, const psl::array<std::shared_future<barrier>>& barriers)
{
	m_Invocables[token - m_TokenOffset].description().dynamic_barriers(barriers);
}
void scheduler::barriers(token token, std::shared_future<barrier>& barrier)
{
	m_Invocables[token - m_TokenOffset].description().dynamic_barriers(barrier);
}

void scheduler::consecutive(token target, psl::array<token> tokens)
{
	m_Invocables[target - m_TokenOffset].description().blocking(tokens);
}