#pragma once
#include <vector>
#include "stdafx_psl.h"
#include <assert.h>
#include "memory/range.h"
#include <list>
#include <algorithm>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <array>
#include <unordered_map>

#define ASYNC_PROFILING

namespace utility
{
	template<typename T>
	class spmc_bounded_queue
	{
	public:
		spmc_bounded_queue(size_t buffer_size)
			: buffer_(new cell_t[buffer_size])
			, buffer_mask_(buffer_size - 1)
		{
			assert((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0));
			for (size_t i = 0; i != buffer_size; i += 1)
				buffer_[i].sequence_.store(i, std::memory_order_relaxed);
			enqueue_pos_.store(0, std::memory_order_relaxed);
			dequeue_pos_.store(0, std::memory_order_relaxed);
		}

		~spmc_bounded_queue()
		{
			delete[] buffer_;
		}

		bool enqueue(T const& data)
		{
			cell_t* cell;
			size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
			for (;;)
			{
				cell = &buffer_[pos & buffer_mask_];
				size_t seq = cell->sequence_.load(std::memory_order_acquire);
				intptr_t dif = (intptr_t)seq - (intptr_t)pos;
				if (dif == 0)
				{
					enqueue_pos_.exchange(pos + 1);
					break;
				}
				else if (dif < 0)
					return false;
				else
					pos = enqueue_pos_.load(std::memory_order_relaxed);
			}

			cell->data_ = data;
			cell->sequence_.store(pos + 1, std::memory_order_release);

			return true;
		}

		bool dequeue(T& data)
		{
			cell_t* cell;
			size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
			for (;;)
			{
				cell = &buffer_[pos & buffer_mask_];
				size_t seq = cell->sequence_.load(std::memory_order_acquire);
				intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
				if (dif == 0)
				{
					if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
						break;
				}
				else if (dif < 0)
					return false;
				else
					pos = dequeue_pos_.load(std::memory_order_relaxed);
			}

			data = cell->data_;
			cell->sequence_.store(pos + buffer_mask_ + 1, std::memory_order_release);

			return true;
		}

		size_t size() const
		{
			return enqueue_pos_.load(std::memory_order_relaxed) - dequeue_pos_.load(std::memory_order_relaxed);
		}

	private:
		struct cell_t
		{
			std::atomic<size_t>     sequence_;
			T                       data_;
		};

		static size_t const         cacheline_size = 64;
		typedef char                cacheline_pad_t[cacheline_size];

		cacheline_pad_t             pad0_;
		cell_t* const               buffer_;
		size_t const                buffer_mask_;
		cacheline_pad_t             pad1_;
		std::atomic<size_t>         enqueue_pos_;
		cacheline_pad_t             pad2_;
		std::atomic<size_t>         dequeue_pos_;
		cacheline_pad_t             pad3_;

		spmc_bounded_queue(spmc_bounded_queue const&);
		void operator = (spmc_bounded_queue const&);
	};
}
namespace psl::async
{
	enum class barrier
	{
		READ	= 0 << 0,
		WRITE	= 1 << 0
	};
	template<typename R, typename... Args>
	class task;
	class scheduler;

	namespace details
	{
		class task_token;
	}

	
	namespace barriers
	{
		class memory
		{
		private:
			template<size_t index, typename T, std::size_t... S>
			void decode_barriers(std::index_sequence<S...>, const T& barrier)
			{
				(..., void(m_Barriers[index][S] = ::memory::range((std::uintptr_t)&std::get<S>(barrier), (std::uintptr_t)&std::get<S>(barrier) + sizeof(decltype(std::get<S>(barrier))))));
				//(..., void(m_Barriers[index][S].end = &std::get<S>(dependencies) + sizeof(decltype(std::get<S>()))));
			}
			
		public:
			memory(const ::memory::range& range, barrier barrier) 
			{
				add(range, barrier);
			}

			/*template<typename R, typename ... Args>
			memory(const psl::evoke<R, Args...>& evocable)
			{
				auto type_qualifiers = utility::templates::get_type_qualifier_v<typename psl::evoke<R, Args...>::tuple_type>{}.value;
				auto type_locations = evocable._parameter_locations();

				for (auto i = 0ui64; i < sizeof...(Args); ++i)
				{
					const auto& type_qualifier{ type_qualifiers[i] };
					const auto& type_location{ type_locations[i] };


					if (type_qualifier == utility::templates::type_qualifier::IS_POINTER || type_qualifier == utility::templates::type_qualifier::IS_CONST_POINTER)
					{
						void* ptrLoc = (void*)type_location.first;
						size_t ptrSize = sizeof(void*);

						void* objLoc = nullptr;
						std::memcpy(&objLoc, ptrLoc, ptrSize);
						size_t objSize = type_location.second;

						barrier dep = (type_qualifier == utility::templates::type_qualifier::IS_CONST_POINTER) ? barrier::READ : barrier::WRITE;
						add(::memory::range{ type_location.first, type_location.first + sizeof(void*) }, dep);
						add(::memory::range{ (std::uintptr_t)objLoc, (std::uintptr_t)objLoc + type_location.second }, dep);
					}
					else if (type_qualifier == utility::templates::type_qualifier::IS_VALUE_REFERENCE || type_qualifier == utility::templates::type_qualifier::IS_CONST_VALUE_REFERENCE)
					{
						barrier dep = (type_qualifier == utility::templates::type_qualifier::IS_CONST_VALUE_REFERENCE) ? barrier::READ : barrier::WRITE;
						add(::memory::range{ type_location.first, type_location.first + type_location.second }, dep);
					}
				}
			}
*/
			template<typename Range_Iterator, typename Barrier_Iterator>
			memory(Range_Iterator&& range_begin, Range_Iterator&& range_end, Barrier_Iterator&& barrier_begin, Barrier_Iterator&& barrier_end)
			{
				add(range_begin, range_end, barrier_begin, barrier_end);
			}

			memory(std::vector<::memory::range>&& read_barriers = {}, std::vector<::memory::range>&& write_barriers = {}) :
				m_Barriers{ std::forward<std::vector<::memory::range>>(read_barriers), std::forward<std::vector<::memory::range>>(write_barriers) }
			{
				std::sort(std::begin(m_Barriers[0]), std::end(m_Barriers[0]));
				std::sort(std::begin(m_Barriers[1]), std::end(m_Barriers[1]));
				static_assert((int)barrier::READ	== 0, "READ barrier should be at slot 0");
				static_assert((int)barrier::WRITE		== 1, "WRITE barrier should be at slot 1");
			}

			template<typename ...V, typename ...U>
			memory(const std::tuple<V...>& read_barriers, const std::tuple<U...>& write_barriers)
			{
				static_assert((int)barrier::READ == 0, "READ barrier should be at slot 0");
				static_assert((int)barrier::WRITE == 1, "WRITE barrier should be at slot 1");
				m_Barriers[(int)barrier::READ].resize(sizeof...(V));
				m_Barriers[(int)barrier::WRITE].resize(sizeof...(U));
				decode_barriers<(int)barrier::READ>(std::index_sequence_for<V...>{}, read_barriers);
				decode_barriers<(int)barrier::WRITE>(std::index_sequence_for<U...>{}, write_barriers);
			}
			
			memory& operator+=(const memory& other)
			{
				if (this != &other)
				{
					for(size_t count = 0; count < m_Barriers.size(); ++count)
					{
						auto size = m_Barriers[count].size();
						m_Barriers[count].resize(m_Barriers[count].size() + other.m_Barriers[count].size());
						std::copy(std::begin(other.m_Barriers[count]), std::end(other.m_Barriers[count]), std::begin(m_Barriers[count]) + size);
						std::inplace_merge(std::begin(m_Barriers[count]), std::begin(m_Barriers[count]) + size, std::end(m_Barriers[count]));
						if (m_Barriers[count].size() > 1)
						{
							for (size_t i = 0u; i < m_Barriers[count].size() - 1; )
							{
								auto& first = m_Barriers[count][i];
								auto& second = m_Barriers[count][i + 1];
								if (first.overlaps(second) || first.end == second.begin)
								{
									first.begin = std::min(first.begin, second.begin);
									first.end = std::max(first.end, second.end);
									m_Barriers[count].erase(std::begin(m_Barriers[count]) + i + 1);
								}
								else
								{
									++i;
								}
							}
						}
					}
				}

				return *this;
			}

			bool can_be_concurrent(const memory& otherMem) const noexcept;
			bool is_dependent_on(const memory& other) const noexcept;

			void add(const ::memory::range& range, barrier barrier);

			template<typename Range_Iterator, typename Barrier_Iterator>
			void add(Range_Iterator&& range_begin, Range_Iterator&& range_end, Barrier_Iterator&& barrier_begin, Barrier_Iterator&& barrier_end)
			{
				auto rangeIt = range_begin;
				auto depIt = barrier_begin;
				while (rangeIt != range_end && depIt != barrier_end)
				{
					size_t dep = (size_t)(*depIt);
					m_Barriers[dep].insert(std::upper_bound(std::begin(m_Barriers[dep]), std::end(m_Barriers[dep]), *rangeIt), *rangeIt);
					//m_Barriers[dep].push_back(*rangeIt);
					rangeIt = std::next(rangeIt);
					depIt = std::next(depIt);
				}
			}

			void remove(const ::memory::range& range, barrier barrier);
			template<typename Range_Iterator, typename Barrier_Iterator>
			void remove(Range_Iterator&& range_begin, Range_Iterator&& range_end, Barrier_Iterator&& barrier_begin, Barrier_Iterator&& barrier_end);

			void clear();
		protected:
			std::array<std::vector<::memory::range>, 2> m_Barriers;
		};

		class task
		{
		public:
			task(details::task_token* owner) : m_Task(owner) {};
			task(details::task_token* owner, std::vector<details::task_token*>&& task_barriers) : m_Task(owner), m_TaskBarrier(std::forward<std::vector<details::task_token*>>(task_barriers)) {};

			bool can_be_concurrent(const task& otherTask) const noexcept;
			bool is_dependent_on(const task& other) const noexcept;

			void add(details::task_token* task);
			void remove(details::task_token* task);

			void clear();
			bool contains(details::task_token* task) const noexcept;

			const std::vector<details::task_token*>& barriers() const noexcept { return m_TaskBarrier; };
		protected:
			std::vector<details::task_token*> m_TaskBarrier;
			details::task_token* m_Task;
		};
	}

	namespace details
	{
		
		class task_token
		{
		public:
			task_token(scheduler& scheduler, std::vector<details::task_token*>&& task_barriers = {}, barriers::memory&& memory_barriers = {}) :
				m_Scheduler(scheduler), m_TaskBarrier(this, std::forward<std::vector<details::task_token*>>(task_barriers)), m_MemBarrier(std::forward<barriers::memory>(memory_barriers)) {};
			virtual ~task_token() = default;
			void execute()
			{
				execute_impl();
				std::unique_lock<std::mutex> lk(m_ResultMut);
				m_Finished = true;
				m_Condition.notify_one();
			}
			bool is_finished() const noexcept { return m_Finished; };

			template<typename R, typename... Args>
			std::shared_ptr<task<R, Args...>> then(R(*function)(Args...), Args&&... args, std::vector<details::task_token*>&& task_barriers = {}, barriers::memory&& memory_barriers = {});

			const barriers::task& task_barriers() const noexcept { return m_TaskBarrier; };
			const barriers::memory& memory_barriers() const noexcept { return m_MemBarrier; };

			void wait_until_finished()
			{
				std::unique_lock<std::mutex> lk(m_ResultMut);
				while (!m_Finished)
				{
					m_Condition.wait(lk, [&] {return m_Finished; });
				}

				assert(m_Finished == true);
			}
		protected:
			template<typename ...Args>
			void add_memory_dependency(const std::tuple<Args...>& target)
			{
				//auto type_qualifiers = utility::templates::get_type_qualifier_v<typename std::tuple<Args>>{}.value;
			}

			virtual void execute_impl() = 0;
			std::condition_variable m_Condition;
			std::mutex m_ResultMut;
		private:
			const barriers::task m_TaskBarrier;
			const barriers::memory m_MemBarrier;
			scheduler& m_Scheduler;
			bool m_Finished{ false };
		};
	}

	template<typename R, typename... Args>
	class task final : public details::task_token
	{
		friend class scheduler;
		using function_t = R(*)(Args...);
		using tuple_t = std::tuple<Args...>;
	public:
		task(scheduler& scheduler, std::vector<details::task_token*>&& task_barriers, barriers::memory&& memory_barriers, R(*function)(Args...), Args&&... args) :
			details::task_token(scheduler, std::forward<std::vector<details::task_token*>>(task_barriers), std::forward<barriers::memory>(memory_barriers)),
			m_Params(std::forward<Args>(args)...), m_Func(function)
		{
			add_memory_dependency(m_Params);
		};

		task(scheduler& scheduler, std::vector<details::task_token*>&& task_barriers, R(*function)(Args...), Args&&... args) :
			details::task_token(scheduler, std::forward<std::vector<details::task_token*>>(task_barriers)),
			m_Params(std::forward<Args>(args)...), m_Func(function)
		{
			add_memory_dependency(m_Params);
		};

		task(scheduler& scheduler, barriers::memory&& memory_barriers, R(*function)(Args...), Args&&... args) :
			details::task_token(scheduler, {}, std::forward<barriers::memory>(memory_barriers)),
			m_Params(std::forward<Args>(args)...), m_Func(function)
		{
			add_memory_dependency(m_Params);
		};

		task(scheduler& scheduler, R(*function)(Args...), Args&&... args) :
			details::task_token(scheduler),
			m_Params(std::forward<Args>(args)...), m_Func(function)
		{
			add_memory_dependency(m_Params);
		};

		R & value()
		{
			return m_Result;
		}

		const R& cvalue() const
		{
			return m_Result;
		}

	private:

		void execute_impl() override
		{
			m_Result = execute_fn(std::index_sequence_for<Args...>{});
		}

		template<std::size_t... S>
		R execute_fn(std::index_sequence<S...>)
		{
			return m_Func(std::forward<Args>(std::get<S>(m_Params)) ...);
		}


		R m_Result;
		function_t m_Func;
		tuple_t m_Params;
	};

	template<typename... Args>
	class task<void, Args...> final : public details::task_token
	{
		friend class scheduler;
		using function_t = void(*)(Args...);
		using tuple_t = std::tuple<Args...>;
		using R = void;
	public:
		task(scheduler& scheduler, void(*function)(Args...), Args&&... args, std::vector<details::task_token*>&& task_barriers = {}, barriers::memory&& memory_barriers = {}) :
			details::task_token(scheduler, std::forward<std::vector<details::task_token*>>(task_barriers), std::forward<barriers::memory>(memory_barriers)),
			m_Params(std::forward<Args>(args)...), m_Func(function)
		{};


		task(scheduler& scheduler, std::vector<details::task_token*>&& task_barriers, barriers::memory&& memory_barriers, R(*function)(Args...), Args&&... args) :
			details::task_token(scheduler, std::forward<std::vector<details::task_token*>>(task_barriers), std::forward<barriers::memory>(memory_barriers)),
			m_Params(std::forward<Args>(args)...), m_Func(function)
		{};

		task(scheduler& scheduler, std::vector<details::task_token*>&& task_barriers, R(*function)(Args...), Args&&... args) :
			details::task_token(scheduler, std::forward<std::vector<details::task_token*>>(task_barriers)),
			m_Params(std::forward<Args>(args)...), m_Func(function)
		{};

		task(scheduler& scheduler, barriers::memory&& memory_barriers, R(*function)(Args...), Args&&... args) :
			details::task_token(scheduler, {}, std::forward<barriers::memory>(memory_barriers)),
			m_Params(std::forward<Args>(args)...), m_Func(function)
		{};

	private:

		void execute_impl() override
		{
			execute_fn(std::index_sequence_for<Args...>{});
		}

		template<std::size_t... S>
		void execute_fn(std::index_sequence<S...>)
		{
			m_Func(std::forward<Args>(std::get<S>(m_Params)) ...);
		}

		function_t m_Func;
		tuple_t m_Params;
	};

	static bool is_finished(const std::vector<details::task_token* >& tasks)
	{
		return std::all_of(std::begin(tasks), std::end(tasks), [](details::task_token* task) {return task->is_finished(); });
	}

	static bool is_any_finished(const std::vector<details::task_token* >& tasks)
	{
		return std::any_of(std::begin(tasks), std::end(tasks), [](details::task_token* task) {return task->is_finished(); });
	}

	static size_t finished_count(const std::vector<details::task_token* >& tasks)
	{
		return std::count_if(std::begin(tasks), std::end(tasks), [](details::task_token* task) {return task->is_finished(); });
	}

	template<typename T>
	static void wait(const std::vector<T* >& tasks)
	{
		for (auto& token : tasks)
			token->wait_until_finished();
	}

	template<typename T>
	static void wait(const std::vector<std::shared_ptr<T> >& tasks)
	{
		for(auto& token : tasks)
			token->wait_until_finished();
	}

	class scheduler final
	{
		enum class worker_state : uint8_t
		{
			DEAD = 0,
			EXECUTING = 1,
			SLEEPING = 2
		};

		struct node
		{
			std::shared_ptr<details::task_token> task;
			// when this reaches zero, I can be added into the queue.
			std::vector<node*> i_depend_on;
			// when i'm executed I need to notify these I'm done.
			std::vector<node*> these_depend_on_me;
		};

		class base_unit
		{
		public:
			base_unit() = default;
			virtual ~base_unit()
			{
				//if (m_Alive)
				{
					// todo: handle error
				}
			}
			base_unit(const base_unit& other) = delete;
			base_unit(base_unit&& other) = delete;
			base_unit& operator=(const base_unit& other) = delete;
			base_unit& operator=(base_unit&& other) = delete;

			void start_thread()
			{
				m_Worker = std::thread(&base_unit::thread_loop, this);
			}

			inline bool is_terminated() const noexcept { return m_Terminate; };
			inline void terminate() noexcept { m_Terminate = true; };
			inline worker_state thread_state() const noexcept { return m_State; };
			inline std::thread& thread() noexcept { return m_Worker; };

			// If the thread is asleep, this wakes it up
			void awake()
			{
				if (!m_ShouldBeAwake)
				{
					std::unique_lock<std::mutex> lk(m_Sleep);
					m_ShouldBeAwake = true;
					m_Conditional.notify_one();
				}
			}

		protected:
			virtual void on_start() {};
			virtual void worker_function() = 0;
			virtual void on_terminate() {};
		private:
			void thread_loop()
			{
				on_start();
				while (!is_terminated())
				{
					m_State = worker_state::SLEEPING;
					sleep_count += 1;
					std::unique_lock<std::mutex> lk(m_Sleep);
					m_Conditional.wait(lk, [&] {return m_ShouldBeAwake; });
					m_ShouldBeAwake = false;
					m_State = worker_state::EXECUTING;
					worker_function();
				}
				m_State = worker_state::DEAD;
				on_terminate();
			}
			bool m_Terminate{ false };

			std::condition_variable m_Conditional;
			bool m_ShouldBeAwake{ false };
			std::mutex m_Sleep;

			volatile worker_state m_State;
			std::thread m_Worker;
			volatile size_t sleep_count{ 0 };
		};

		class job_divider;

		class unit : public base_unit
		{
			friend class job_divider;
		public:
			unit(job_divider* job_thread) : base_unit(), m_Consumer(job_thread->m_Execution) {};

		private:
			void worker_function() override
			{
				node* processing = nullptr;
				while (m_Consumer.dequeue(processing))
				{
					processing->task->execute();
					m_Finalized.push(processing);
				}
			}

			utility::spmc_bounded_queue<node*>& m_Consumer;
			std::queue<node*> m_Finalized;
		};

		class job_divider final : public base_unit
		{
			friend class unit;
		public:
			job_divider(size_t workers) : m_Execution(1024)
			{
				m_Workers.reserve(workers);
				for (auto i = 0; i < workers; ++i)
				{
					m_Workers.emplace_back(new unit(this));
				}
			};
			~job_divider()
			{
			}

			void add(std::shared_ptr<details::task_token>& task)
			{
				std::unique_lock<std::mutex> lk(m_Adding);
				m_ToBeAdded.push_back(task);				
				//awake();
			}

			void force_finish()
			{
				//while (m_HasWork)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}

			void manual_start()
			{
				on_start();
			}

			void manual_awake()
			{
				worker_function();
			}

			void manual_terminate()
			{
				on_terminate();
			}
		private:

			// local thread loop
			void worker_function() override
			{
				std::vector<std::shared_ptr<details::task_token>> toBeAdded;

				{
					std::unique_lock<std::mutex> lk(m_Adding);
					toBeAdded = m_ToBeAdded;
					m_ToBeAdded.clear();
				}

				bool hasEnqueued = false;

				for (auto task : toBeAdded)
				{
					node* n = new node();
					n->task = task;
					for (auto& barrier : task->task_barriers().barriers())
					{
						if (auto it = m_Tasks.find(barrier); it != std::end(m_Tasks))
						{
							n->i_depend_on.emplace_back(it->second);
							it->second->these_depend_on_me.emplace_back(n);
						}
					}

					if (n->i_depend_on.size() == 0 && task->memory_barriers().can_be_concurrent(m_ExecutionBarrier))
					{
						m_ExecutionBarrier += task->memory_barriers();
						m_Execution.enqueue(n);
						hasEnqueued = true;
					}
					else
					{
						m_Tasks[task.get()] = n;
					}
				}

				if (hasEnqueued)
				{
					for (auto& worker : m_Workers)
					{
						worker->awake();
					}
				}
			}

			void on_start() override
			{
				for (auto& it : m_Workers)
					it->start_thread();
			}

			void on_terminate() override
			{
				for (auto& it : m_Workers)
				{
					it->terminate();
					it->awake();
				}

				for (auto& it : m_Workers)
				{
					it->thread().join();
				}
			}

			std::vector<std::shared_ptr<details::task_token>> m_ToBeAdded;
			std::unordered_map<details::task_token*, node*> m_Tasks;
			utility::spmc_bounded_queue<node*> m_Execution;
			barriers::memory m_ExecutionBarrier;
			std::vector<unit*> m_Workers;

			std::mutex m_Adding;

		};

	public:
		scheduler(size_t workers = 2) : m_JobThread(workers)
		{
			m_JobThread.start_thread();
			m_JobThread.awake();
		}

		~scheduler()
		{
			m_JobThread.force_finish();
			m_JobThread.terminate();
			m_JobThread.awake();
			m_JobThread.thread().join();


		}

		void execute()
		{
			m_JobThread.awake();
		}

		template<typename R, typename... Args>
		std::shared_ptr<task<R, Args...>> schedule(std::vector<details::task_token*>&& task_barriers, barriers::memory&& memory_barriers, R(*function)(Args...), Args&&... args)
		{
			//memory_barriers += { evocable };

			std::shared_ptr<details::task_token> task_handle{ std::make_shared<task<R, Args...>>(*this, function, std::forward<Args>(args)...,std::forward<std::vector<details::task_token*>>(task_barriers), std::forward<barriers::memory>(memory_barriers)) };
			m_JobThread.add(task_handle);
			return std::static_pointer_cast<task<R, Args...>>(task_handle);
		}


		template<typename R, typename... Args>
		std::shared_ptr<task<R, Args...>> schedule(R(*function)(Args...), Args... args)
		{
			//memory_barriers += { evocable };

			std::shared_ptr<details::task_token> task_handle{ std::make_shared<task<R, Args...>>(*this, function, std::forward<Args>(args)...) };
			m_JobThread.add(task_handle);
			return std::static_pointer_cast<task<R, Args...>>(task_handle);
		}
	private:

		job_divider m_JobThread;
	};
}


template<typename R, typename ...Args>
inline std::shared_ptr<psl::async::task<R, Args...>> psl::async::details::task_token::then(R(*function)(Args...), Args&&... args, std::vector<details::task_token*>&& task_barriers, barriers::memory&& memory_barriers)
{
	task_barriers.push_back(this);
	return m_Scheduler.schedule(std::forward(function), std::forward<Args>(args)..., std::forward<std::vector<details::task_token*>>(task_barriers), std::forward<barriers::memory>(memory_barriers));
	//return new psl::async::task<O, Args...>(m_Scheduler, std::forward<psl::evoke<O, Args...>>(evocable), std::forward<std::vector<details::task_token*>>(task_barriers), std::forward<barriers::memory>(memory_barriers));
}
