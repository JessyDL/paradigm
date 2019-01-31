#include "task.h"
#include "array_view.h"
#include <optional>
using namespace psl::async;

scheduler::scheduler(std::optional<size_t> prefered_workers, bool force)
{
	if(force && prefered_workers.has_value())
		m_WorkerCount = prefered_workers.value();
	else
		m_WorkerCount = std::min<size_t>(prefered_workers.value_or(std::thread::hardware_concurrency()),
										 std::thread::hardware_concurrency());
}

[[nodiscard]] std::future<void> scheduler::execute(launch policy) {
	struct enqueued_task
	{
		enqueued_task() = default;
		enqueued_task(scheduler::description description, token_t token, size_t heuristic)
			: description(description), token(token), heuristic(heuristic){};

		~enqueued_task()
		{ 

		}

		enqueued_task(enqueued_task&& other) = default;
		enqueued_task& operator=(enqueued_task&& other) = default;

		scheduler::description description;
		std::future<void> future;
		token_t token;
		size_t heuristic{0};
	};

	auto calculate_heuristic = [](token_t token,
								  const std::unordered_map<token_t, scheduler::description>& tasklist) -> size_t {
		size_t heuristic		= 0;
		const auto& description = tasklist.at(token);
		for(const auto& barrier : description.barriers)
		{
			if(barrier.type() != barrier_type::READ_WRITE) continue;


			for(const auto& [task_token, task_description] : tasklist)
			{
				if(task_token == token) continue;

				size_t barrier_conflict{
					size_t(std::count_if(std::begin(task_description.barriers), std::end(task_description.barriers),
										 [&barrier](const async::barrier& b) { return b.conflicts(barrier); }))};
				barrier_conflict *= 2;
				heuristic += barrier_conflict * barrier_conflict;
			}
		}

		// now we go through all the other tasks, and find those who depend on me, and increase my heuristic.
		size_t my_dependencies{0};
		for(const auto& [task_token, task_description] : tasklist)
		{
			if(task_token == token) continue;

			my_dependencies += (std::find(std::begin(task_description.tokens), std::end(task_description.tokens),
										  token) != std::end(task_description.tokens))
								   ? 1
								   : 0;
		}
		my_dependencies *= 2;
		heuristic += my_dependencies * my_dependencies * my_dependencies;

		return heuristic;
	};

	auto is_invalid = [](const enqueued_task& task, psl::array_view<enqueued_task> preceding_tasks,
						 psl::array_view<std::optional<enqueued_task>> inflight_tasks,
						 const std::vector<barrier>& barriers) -> bool {
		const auto& description = task.description;
		return (std::any_of(std::begin(description.tokens), std::end(description.tokens),
							[&](token_t t) {
								return std::find_if(std::begin(preceding_tasks), std::end(preceding_tasks),
													[t](const enqueued_task& pre_task) {
														return pre_task.token == t;
													}) != std::end(preceding_tasks) ||
									   std::find_if(std::begin(inflight_tasks), std::end(inflight_tasks),
													[t](const std::optional<enqueued_task>& inflight_task) {
														return inflight_task.has_value() &&
															   inflight_task.value().token == t;
													}) != std::end(inflight_tasks);
							}) ||

				std::any_of(std::begin(description.barriers), std::end(description.barriers),
							[&barriers](const barrier& b) {
								return std::any_of(std::begin(barriers), std::end(barriers),
												   [&b](const barrier& lock_b) { return b.conflicts(lock_b); });
							}));
	};

	auto execution_impl = [is_invalid, calculate_heuristic](std::unordered_map<token_t, description> tasklist,
															size_t workerCount) {
		if(tasklist.size() == 0) return;
		std::vector<std::optional<enqueued_task>> scheduled{};
		std::vector<barrier> locks{};
		scheduled.resize(workerCount);

		std::vector<enqueued_task> enqueued_tasks;
		enqueued_tasks.reserve(tasklist.size());
		for(const auto& [token, descr] : tasklist)
		{
			enqueued_tasks.emplace_back(enqueued_task{descr, token, calculate_heuristic(token, tasklist)});
		}
		std::sort(std::begin(enqueued_tasks), std::end(enqueued_tasks),
				  [](const enqueued_task& a, const enqueued_task& b) { return a.heuristic > b.heuristic; });

		tasklist.clear();

		std::vector< details::task_base*> finalized_tasks;
		while(enqueued_tasks.size() > 0)
		{
			for(size_t i = 0; i < scheduled.size(); ++i)
			{
				if((!scheduled[i].has_value() || scheduled[i].value().future.valid()))
				{
					if(scheduled[i].has_value())
					{
						auto& descr{scheduled[i].value().description};

						finalized_tasks.emplace_back(descr.task);
						for(const auto& b : descr.barriers)
							locks.erase(std::find(std::begin(locks), std::end(locks), b));
						scheduled[i] = std::nullopt;
					}

					auto index = size_t{0};
					for(auto& enqueued_task : enqueued_tasks)
					{
						++index;
						if(is_invalid(enqueued_task, enqueued_tasks, scheduled, locks)) continue;

						locks.insert(std::end(locks), std::begin(enqueued_task.description.barriers),
									 std::end(enqueued_task.description.barriers));

						enqueued_task.future = std::async(std::launch::async, &details::task_base::operator(),
														  enqueued_task.description.task);
						scheduled[i]		 = std::move(enqueued_task);

						enqueued_tasks.erase(std::next(std::begin(enqueued_tasks), index - 1));
						break;
					}
				}

				if(enqueued_tasks.size() == 0) break;
			}
		}

		for(size_t i = 0; i < scheduled.size(); ++i)
		{
			if(scheduled[i].has_value())
			{
				scheduled[i].value().future.wait();
				finalized_tasks.emplace_back(scheduled[i].value().description.task);
				scheduled[i] = std::nullopt;
			}
		}

		for(auto task : finalized_tasks)
		{
			delete(task);
		}
	};

	auto launch_policy	= (policy == launch::async) ? std::launch::async : std::launch::deferred;
	std::future<void> res = std::async(launch_policy, execution_impl, m_TaskList, m_WorkerCount * 2);

	m_TaskList.clear();

	if(policy == launch::immediate) res.wait();

	return res;
}