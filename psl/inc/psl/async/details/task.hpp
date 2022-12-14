#pragma once
#include <functional>
#include <future>

namespace psl::async::details {
class task_base {
  public:
	virtual ~task_base()	  = default;
	virtual void operator()() = 0;
};


template <typename R, typename Storage = std::function<void()>, typename Future = std::future<R>>
class task final : public task_base {
	using Actual_Storage = typename std::remove_reference<Storage>::type;

  public:
	task(Storage&& invocable) : m_Invocable(std::forward<decltype(invocable)>(invocable)) {};
	virtual ~task() = default;
	Future future() noexcept { return m_Promise.get_future(); }

	void operator()() override { m_Promise.set_value(std::move(std::invoke(m_Invocable))); }

  private:
	Actual_Storage m_Invocable;
	std::promise<R> m_Promise;
};

template <typename R, typename Storage>
class task<R, Storage, void> final : public task_base {
	using Actual_Storage = typename std::remove_reference<Storage>::type;

  public:
	task(Storage&& invocable) : m_Invocable(std::forward<decltype(invocable)>(invocable)) {};
	virtual ~task() = default;

	void operator()() override { std::invoke(m_Invocable); }

  private:
	Actual_Storage m_Invocable;
};
}	 // namespace psl::async::details
