#pragma once
#include <vector>
#include "assertions.h"
#include "memory/range.h"
#include <list>
#include <algorithm>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <array>
#include <unordered_map>
#include "template_utils.h"
#include <future>
#include <optional>

#define ASYNC_PROFILING

//namespace utility
//{
//	template <typename T>
//	class spmc_bounded_queue
//	{
//	  public:
//		spmc_bounded_queue(size_t buffer_size) : buffer_(new cell_t[buffer_size]), buffer_mask_(buffer_size - 1)
//		{
//			assert((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0));
//			for(size_t i = 0; i != buffer_size; i += 1) buffer_[i].sequence_.store(i, std::memory_order_relaxed);
//			enqueue_pos_.store(0, std::memory_order_relaxed);
//			dequeue_pos_.store(0, std::memory_order_relaxed);
//		}
//
//		~spmc_bounded_queue() { delete[] buffer_; }
//
//		bool enqueue(T const& data)
//		{
//			cell_t* cell;
//			size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
//			for(;;)
//			{
//				cell		 = &buffer_[pos & buffer_mask_];
//				size_t seq   = cell->sequence_.load(std::memory_order_acquire);
//				intptr_t dif = (intptr_t)seq - (intptr_t)pos;
//				if(dif == 0)
//				{
//					enqueue_pos_.exchange(pos + 1);
//					break;
//				}
//				else if(dif < 0)
//					return false;
//				else
//					pos = enqueue_pos_.load(std::memory_order_relaxed);
//			}
//
//			cell->data_ = data;
//			cell->sequence_.store(pos + 1, std::memory_order_release);
//
//			return true;
//		}
//
//		bool dequeue(T& data)
//		{
//			cell_t* cell;
//			size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
//			for(;;)
//			{
//				cell		 = &buffer_[pos & buffer_mask_];
//				size_t seq   = cell->sequence_.load(std::memory_order_acquire);
//				intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
//				if(dif == 0)
//				{
//					if(dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
//				}
//				else if(dif < 0)
//					return false;
//				else
//					pos = dequeue_pos_.load(std::memory_order_relaxed);
//			}
//
//			data = cell->data_;
//			cell->sequence_.store(pos + buffer_mask_ + 1, std::memory_order_release);
//
//			return true;
//		}
//
//		size_t size() const
//		{
//			return enqueue_pos_.load(std::memory_order_relaxed) - dequeue_pos_.load(std::memory_order_relaxed);
//		}
//
//	  private:
//		struct cell_t
//		{
//			std::atomic<size_t> sequence_;
//			T data_;
//		};
//
//		static size_t const cacheline_size = 64;
//		typedef char cacheline_pad_t[cacheline_size];
//
//		cacheline_pad_t pad0_;
//		cell_t* const buffer_;
//		size_t const buffer_mask_;
//		cacheline_pad_t pad1_;
//		std::atomic<size_t> enqueue_pos_;
//		cacheline_pad_t pad2_;
//		std::atomic<size_t> dequeue_pos_;
//		cacheline_pad_t pad3_;
//
//		spmc_bounded_queue(spmc_bounded_queue const&);
//		void operator=(spmc_bounded_queue const&);
//	};
//} // namespace utility

namespace psl::async
{
	template <typename R, typename... Args>
	class task;

	namespace details
	{
		template <typename R, typename T>
		struct transform_to_task_impl
		{};

		template <typename Fn>
		struct transform_to_task : public transform_to_task_impl<typename psl::templates::func_traits<Fn>::result_t,
																 typename psl::templates::func_traits<Fn>::arguments_t>
		{};

		template <typename R, typename... Ts>
		struct transform_to_task_impl<R, std::tuple<Ts...>>
		{
			using type = task<R, Ts...>;
		};

		class task_base
		{
		  public:
			  virtual ~task_base() = default;
			virtual void operator()() = 0;
		};
	} // namespace details

	enum class barrier_type : uint8_t
	{
		READ	   = 0,
		READ_WRITE = 1
	};

	enum class launch : uint8_t
	{
		immediate = 0,
		async	 = 1,
		deferred  = 2
	};
	class barrier
	{
	  public:
		using location_t   = std::uintptr_t;
		barrier() noexcept = default;
		barrier(location_t begin, location_t end, barrier_type type = barrier_type::READ)
			: m_Begin(begin), m_End(end), m_Type(type){};

		bool operator==(const barrier& other) const noexcept
		{
			return m_Begin == other.m_Begin && m_End == other.m_End && m_Type == other.m_Type;
		}
		bool operator!=(const barrier& other) const noexcept
		{
			return m_Begin != other.m_Begin || m_End != other.m_End || m_Type != other.m_Type;
		}
		location_t begin() const noexcept { return m_Begin; }
		location_t end() const noexcept { return m_End; }
		location_t size() const noexcept { return m_End - m_Begin; }
		barrier_type type() const noexcept { return m_Type; }

		void begin(location_t location) noexcept { m_Begin = location; };
		void end(location_t location) noexcept { m_End = location; };
		void type(barrier_type type) noexcept { m_Type = type; }

		void move(location_t location) noexcept
		{
			m_End   = size() + location;
			m_Begin = location;
		}
		void resize(location_t new_size) noexcept { m_End = m_Begin + new_size; };

		bool conflicts(const barrier& other) const noexcept
		{
			if((m_Type == barrier_type::READ && m_Type == other.m_Type) || !overlaps(other)) return false;
			return true;
		}

		bool overlaps(const barrier& other) const noexcept
		{
			return (other.m_Begin >= m_Begin && other.m_Begin < m_End) ||
				   (other.m_End > m_Begin && other.m_End <= m_End);
		}

	  private:
		barrier_type m_Type;
		location_t m_Begin{0};
		location_t m_End{0};
	};


	template <typename R, typename... Args>
	class task : public details::task_base
	{
	  public:
		template <typename Fn>
		task(Fn&& invocable, Args&&... arguments)
			: m_Invocable(std::forward<Fn>(invocable)), m_Arguments(std::forward<Args>(arguments)...)
		{}

		virtual ~task() = default;
		std::future<R> future() noexcept { return m_Promise.get_future(); }

		void operator()() override 
		{ 
			if constexpr(std::is_same<R, void>::value)
			{
				std::apply(m_Invocable, m_Arguments);
				m_Promise.set_value();
			}
			else
			{
				m_Promise.set_value(std::move(std::apply(m_Invocable, m_Arguments))); 
			}
		}

	  private:
		std::function<R(Args...)> m_Invocable;
		std::tuple<Args...> m_Arguments;
		std::promise<R> m_Promise;
	};

	template <typename R, typename T, typename... Args>
	class member_task : public details::task_base
	{
	public:
		template <typename Fn>
		member_task(Fn&& invocable, T* ptr, Args&&... arguments)
			: m_Invocable(std::forward<Fn>(invocable)), m_Ptr(ptr), m_Arguments(std::forward<Args>(arguments)...)
		{}
		virtual ~member_task() = default;

		std::future<R> future() noexcept { return m_Promise.get_future(); }

		void operator()() override
		{
			if constexpr(std::is_same<R, void>::value)
			{
				std::apply(m_Invocable, m_Ptr, m_Arguments);
				m_Promise.set_value();
			}
			else
			{
				m_Promise.set_value(std::move(std::apply(m_Invocable, m_Ptr, m_Arguments)));
			}
		}

	private:
		std::function<R(Args...)> m_Invocable;
		std::tuple<Args...> m_Arguments;
		T* m_Ptr;
		std::promise<R> m_Promise;
	};

	using token_t = std::uintptr_t;

	template <typename T>
	class task_token
	{


	  private:
		std::future<T> m_Future;
	};

	/*class worker
	{
	  public:
		  worker() = default;
		  worker(utility::spmc_bounded_queue<details::task_base*>* consumer) : m_Consumer(consumer), m_Worker{std::thread(&worker::thread_loop, this)} {};
		  ~worker()
		  {
			  if (m_State != state_t::DEAD)
			  {
				  //throw std::runtime_error("did not clean up worker thread gracefully");
			  }
		  }
		  worker(const worker& other) = delete;
		  worker(worker&& other) = delete;
		  worker& operator=(const worker& other) = delete;
		  worker& operator=(worker&& other) = delete;

		enum class state_t : uint8_t
		{
			DEAD	  = 0,
			EXECUTING = 1,
			SLEEPING  = 2
		};

		void awake() noexcept
		{
			if(!m_ShouldBeAwake)
			{
				std::unique_lock<std::mutex> lk(m_Sleep);
				m_ShouldBeAwake = true;
				m_Conditional.notify_one();
			}
		}

		state_t state() const noexcept { return m_State; }

		bool terminated() const noexcept { return m_Terminate; }
		void terminate() noexcept { m_Terminate = true; }

	  private:
		void thread_loop()
		{
			while(!m_Terminate)
			{
				m_State = state_t::SLEEPING;
				std::unique_lock<std::mutex> lk(m_Sleep);
				m_Conditional.wait(lk, [&] { return m_ShouldBeAwake; });
				m_ShouldBeAwake = false;
				m_State			= state_t::EXECUTING;
				details::task_base* processing = nullptr;
				while(m_Consumer->dequeue(processing))
				{
					processing->operator()();
					m_Finalized.push(processing);
				}
			}
			m_State = state_t::DEAD;
		}

		bool m_Terminate{false};

		std::condition_variable m_Conditional;
		bool m_ShouldBeAwake{false};
		std::mutex m_Sleep;

		volatile state_t m_State{state_t::SLEEPING};
		utility::spmc_bounded_queue<details::task_base*>* m_Consumer{nullptr};
		std::thread m_Worker;


		std::queue<details::task_base*> m_Finalized;
	};*/

	class scheduler
	{
		struct description
		{
			description() = default;
			description(details::task_base* task) : task(task){};
			description(const description&) = default;
			description& operator=(const description&) = default;
			description(description&& other)
				: task(std::move(other.task)), barriers(std::move(other.barriers)), tokens(std::move(other.tokens))
			{
				other.task = nullptr;
			}

			description& operator=(description&& other)
			{
				if(this != &other)
				{
					task	   = std::move(other.task);
					barriers   = std::move(other.barriers);
					tokens	 = std::move(other.tokens);
					other.task = nullptr;
				}
				return *this;
			}
			details::task_base* task{nullptr};
			std::vector<barrier> barriers{};
			std::vector<token_t> tokens{};
			std::vector<token_t> blockers{};
		};


	  public:
		  scheduler(std::optional<size_t> prefered_workers = std::nullopt, bool force = false);
		/*template <typename Fn, typename... Args>
		[[nodiscard]] typename std::enable_if<
			!std::is_member_function_pointer<Fn>::value,
			std::pair<token_t, std::future<typename psl::templates::func_traits<Fn>::result_t>>>::type
		schedule(Fn&& function, Args&&... arguments)
		{
			using return_t = typename psl::templates::func_traits<Fn>::result_t;
			using task_t   = task<return_t, Args&&...>;

			auto ret = m_TaskList.emplace(
				m_TokenGenerator++,
				description{new task_t{std::forward<Fn>(function), std::forward<Args>(arguments)...}});
			task_t& task{*(task_t*)(ret.first->second.task)};
			return std::pair{ret.first->first, task.future()};
		}*/

		template <typename Fn, typename... Args>
		[[nodiscard]] auto schedule(Fn&& function, Args&&... arguments)
		{
			using fn_t = Fn;
			if constexpr(!std::is_member_function_pointer<Fn>::value)
			{
				using return_t = typename psl::templates::func_traits<fn_t>::result_t;
				using task_t = task<return_t, Args&&...>;

				auto ret = m_TaskList.emplace(
					m_TokenGenerator++,
					description{new task_t{function, std::forward<Args>(arguments)...}});
				task_t& task{*(task_t*)(ret.first->second.task)};
				return std::pair{ret.first->first, task.future()};
			}
			else
			{
				using return_t = typename psl::templates::func_traits<fn_t>::result_t;
				using task_t = member_task<return_t, Args&&...>;

				auto func = [function](Args&&... args) -> return_t
				{
					return std::invoke(function, std::forward<Args>(args)...);
				};

				auto ret =
					m_TaskList.emplace(m_TokenGenerator++, description{new task_t{func, std::forward<Args>(arguments)...}});
				task_t& task{*(task_t*)(ret.first->second.task)};
				return std::pair{ret.first->first, task.future()};
			}
		}

		/*template <typename Fn, typename T, typename... Args>
		[[nodiscard]] typename std::enable_if<
			std::is_member_function_pointer<Fn>::value,
			std::pair<token_t, std::future<typename psl::templates::func_traits<Fn>::result_t>>>::type
		schedule(Fn&& function, T* target, Args&&... arguments)
		{
			using return_t = typename psl::templates::func_traits<Fn>::result_t;
			using task_t   = task<return_t, Args&&...>;

			auto func = [function, target](Args&&... args) -> return_t {
				return std::invoke(function, target, std::forward<Args>(args)...);
			};

			auto ret =
				m_TaskList.emplace(m_TokenGenerator++, description{new task_t{func, std::forward<Args>(arguments)...}});
			task_t& task{*(task_t*)(ret.first->second.task)};
			return std::pair{ret.first->first, task.future()};
		}*/

		template <typename Fn>
		[[nodiscard]] std::pair<token_t, std::future<typename psl::templates::func_traits<Fn>::result_t>>
		schedule(Fn&& function)
		{
			using return_t = typename psl::templates::func_traits<Fn>::result_t;
			using task_t   = typename details::transform_to_task<Fn>::type;

			auto ret = m_TaskList.emplace(m_TokenGenerator++, description{new task_t{std::forward<Fn>(function)}});
			task_t& task{*(task_t*)(ret.first->second.task)};
			return std::pair{ret.first->first, task.future()};
		}

		void dependency(token_t token, barrier&& barrier) noexcept;

		void dependency(token_t token, std::vector<barrier>&& barriers) noexcept;

		void dependency(token_t token, token_t other) noexcept;

		void dependency(token_t token, std::vector<token_t>&& other) noexcept;

		/// \brief signifies which tasks cannot run in parallel
		///
		/// Sometimes tasks might access shared resources, or mutate global state
		/// yet have no clear "dependency" on one another.
		/// This method forces these tasks to run sequential and disallows the 
		/// others to run when one of them is running.
		void enforce_sequential(std::vector<token_t> tokens) noexcept;

		/// \brief Start executing all scheduled tasks
		///
		/// When invoking execute, all scheduled tasks will be enqueued on the worker task queues,
		/// currently existing tokens get invalidated, so tasks can no longer be edited.
		/// Re-using a token that was assigned to a task before executing results in undefined behaviour.
		/// At best, an immediate crash, at worst "it works".
		/// \param[in] blocking when true, the invoking thread will wait for the results before continuing.
		[[nodiscard]] std::future<void> execute(launch policy = launch::immediate);

		size_t workers() const noexcept{return m_WorkerCount;};
	  private:
		  size_t m_WorkerCount {4};
		std::unordered_map<token_t, description> m_TaskList;
		token_t m_TokenGenerator{0};
	};


} // namespace psl::async2