#include "task.h"
#include <optional>
using namespace psl::async;


bool barriers::memory::is_dependent_on(const barriers::memory& other) const noexcept
{
	for(auto& dep : m_Barriers)
	{
		for(auto& read_dep : dep)
		{
			for(auto& other_write_dep : other.m_Barriers[1])
			{
				if(read_dep.overlaps(other_write_dep)) return true;
			}
		}
	}
	return false;
}

void barriers::memory::add(const ::memory::range& range, barrier barrier)
{
	size_t dep = (size_t)barrier;
	m_Barriers[dep].insert(std::upper_bound(std::begin(m_Barriers[dep]), std::end(m_Barriers[dep]), range), range);
	// m_Barriers[dep].push_back(range);
}


void barriers::memory::remove(const ::memory::range& range, barrier barrier)
{
	size_t dep = (size_t)barrier;
	auto it	= std::find(std::begin(m_Barriers[dep]), std::end(m_Barriers[dep]), range);
	if(it != std::end(m_Barriers[dep])) m_Barriers[dep].erase(it);
}

void barriers::memory::clear()
{
	for(auto& it : m_Barriers)
	{
		it.clear();
	}
}

bool barriers::memory::can_be_concurrent(const barriers::memory& otherMem) const noexcept
{
	return !is_dependent_on(otherMem) && !otherMem.is_dependent_on(*this);
}

void barriers::task::add(details::task_token* task) { m_TaskBarrier.push_back(task); }

void barriers::task::remove(details::task_token* task)
{
	m_TaskBarrier.erase(std::find(std::begin(m_TaskBarrier), std::end(m_TaskBarrier), task));
}
void barriers::task::clear() { m_TaskBarrier.clear(); }


bool barriers::task::contains(details::task_token* task) const noexcept
{
	return std::find(std::begin(m_TaskBarrier), std::end(m_TaskBarrier), task) != std::end(m_TaskBarrier);
}

bool barriers::task::can_be_concurrent(const barriers::task& otherTask) const noexcept
{
	return !is_dependent_on(otherTask) && !otherTask.is_dependent_on(*this);
}
bool barriers::task::is_dependent_on(const barriers::task& other) const noexcept
{
	return std::find(std::begin(m_TaskBarrier), std::end(m_TaskBarrier), other.m_Task) == std::end(m_TaskBarrier);
}


[[nodiscard]] std::future<void> psl::async2::scheduler::execute(launch policy) {
	struct enqueued_task
	{
		enqueued_task() = default;
		enqueued_task(psl::async2::scheduler::description description,
					  std::future<void>&& future) : description(description), future(std::move(future)) {};

		~enqueued_task()
		{
			delete(description.task);
		}

		enqueued_task(enqueued_task&& other) = default;
		enqueued_task& operator=(enqueued_task&& other) = default;

		psl::async2::scheduler::description description;
		std::future<void> future;
	};

	auto is_invalid = [](const description& descr, const std::unordered_map<token_t, description>& tasklist, const std::vector<barrier>& barriers) -> bool {

		return (std::any_of(std::begin(descr.tokens), std::end(descr.tokens),
					[&](token_t t) { return tasklist.find(t) != std::end(tasklist); }) ||

			std::any_of(std::begin(descr.barriers), std::end(descr.barriers),
						[&barriers](const barrier& b)
						{
							return std::any_of(std::begin(barriers), std::end(barriers),
											   [&b](const barrier& lock_b) { return b.conflicts(lock_b); });
						}));
	};

	auto execution_impl2 = [is_invalid](std::unordered_map<token_t, description> tasklist) {
		if(tasklist.size() == 0)
			return;
		std::vector<std::optional<enqueued_task>> scheduled{};
		std::vector<barrier> locks{};
		scheduled.resize(std::max(size_t{std::thread::hardware_concurrency()}, size_t{4}));

		while(tasklist.size() > 0)
		{
			for(size_t i = 0; i < scheduled.size(); ++i)
			{
				if((!scheduled[i].has_value() || scheduled[i].value().future.valid()))
				{
					if(scheduled[i].has_value())
					{
						auto& descr {scheduled[i].value().description};

						for(const auto& b : descr.barriers)
							locks.erase(std::find(std::begin(locks), std::end(locks), b));
					}
					for(const auto& [token, descr] : tasklist)
					{
						if(is_invalid(descr, tasklist, locks)) continue;

						auto future = std::async(std::launch::async, &details::task_base::operator(), descr.task);

						scheduled[i] = enqueued_task{
							descr, std::move(future)};

						locks.insert(std::end(locks), std::begin(descr.barriers), std::end(descr.barriers));

						tasklist.erase(token);
						break;
					}
				}

				if(tasklist.size()== 0)
					break;
			}
		}

		for(size_t i = 0; i < scheduled.size(); ++i)
		{
			if(scheduled[i].has_value())
			{
				scheduled[i].value().future.wait();
			}
		}
	};

	auto execution_impl = [](std::unordered_map<token_t, description> tasklist) {
		while(tasklist.size() > 0)
		{
			std::vector<std::pair<token_t, std::future<void>>> scheduled{};
			std::vector<barrier> locks{};


			for(auto& [token, description] : tasklist)
			{
				// IF any_of the tasks dependency tokens are still not finalized
				// OR
				// IF any_of the barriers conflicts with already scheduled tasks
				// THEN don't enqueue.
				if(std::any_of(std::begin(description.tokens), std::end(description.tokens),
							   [&](token_t t) { return tasklist.find(t) != std::end(tasklist); }) ||

				   std::any_of(std::begin(description.barriers), std::end(description.barriers),
							   [&locks](const barrier& b) {
								   return std::any_of(std::begin(locks), std::end(locks),
													  [&b](const barrier& lock_b) { return b.conflicts(lock_b); });
							   }))
					continue;


				locks.insert(std::end(locks), std::begin(description.barriers), std::end(description.barriers));
				scheduled.emplace_back(std::pair{
					token, std::async(std::launch::async, &details::task_base::operator(), description.task)});
			}


			std::for_each(std::begin(scheduled), std::end(scheduled),
						  [&](const std::pair<token_t, std::future<void>>& pair) {
							  pair.second.wait();
							  tasklist.erase(pair.first);
						  });
		}
	};

	auto launch_policy = (policy == launch::async) ? std::launch::async : std::launch::deferred;
	auto res		   = std::async(launch_policy, execution_impl2, m_TaskList);
	m_TaskList.clear();
	if(policy == launch::immediate) res.wait();
	return res;
}