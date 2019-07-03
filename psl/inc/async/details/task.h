#pragma once
#include <functional>
#include <future>

namespace psl::async2::details
{
	class task_base
	{
	  public:
		virtual ~task_base()	  = default;
		virtual void operator()() = 0;
	};


	template <typename R, typename Storage = std::function<void(std::promise<R>&)>, typename Future = std::future<R>>
	class task final : public task_base
	{
	  public:
		task(Storage&& invocable)
			: m_Invocable(std::move(invocable)){};
		virtual ~task() = default;
		Future future() noexcept 
		{ 
			return m_Promise.get_future();
		}

		void operator()() override
		{
			std::invoke(m_Invocable, std::ref(m_Promise));
		}

	  private:
		Storage m_Invocable;
		std::promise<R> m_Promise;
	};
} // namespace psl::async2::details