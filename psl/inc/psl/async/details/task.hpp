#pragma once
#include <functional>
#include <future>

namespace psl::async {
enum class result { success, failure };
}

namespace psl::async::details {
class task_base {
  public:
	virtual ~task_base()	  = default;
	virtual void operator()() = 0;
	virtual bool is_retryable() const noexcept {
		return false;
	};

	virtual bool has_failed() const noexcept {
		return false;
	}

	virtual void reset() noexcept {};
};


template <typename R, typename Storage = std::function<void()>, typename Future = std::future<R>>
class task final : public task_base {
	using Actual_Storage = typename std::remove_reference<Storage>::type;

  public:
	task(Storage&& invocable) : m_Invocable(std::forward<decltype(invocable)>(invocable)) {};
	virtual ~task() = default;
	Future future() noexcept {
		return m_Promise.get_future();
	}

	void operator()() override {
		if constexpr(std::is_same<psl::async::result, R>::value) {
			auto res = std::invoke(m_Invocable);
			if(res == psl::async::result::failure) {
				m_HasFailed = true;
			} else {
				m_Promise.set_value(std::move(res));
			}
		} else {
			m_Promise.set_value(std::move(std::invoke(m_Invocable)));
		}
	}

	bool is_retryable() const noexcept override {
		if constexpr(std::is_same<psl::async::result, R>::value) {
			return true;
		} else {
			return false;
		}
	}

	bool has_failed() const noexcept override {
		if constexpr(std::is_same<psl::async::result, R>::value) {
			return m_HasFailed;
		} else {
			return false;
		}
	}

	void reset() noexcept override {
		if constexpr(std::is_same<psl::async::result, R>::value) {
			m_HasFailed = false;
		}
	}

  private:
	Actual_Storage m_Invocable;
	std::promise<R> m_Promise;
	bool m_HasFailed {false};
};

template <typename R, typename Storage>
class task<R, Storage, void> final : public task_base {
	using Actual_Storage = typename std::remove_reference<Storage>::type;

  public:
	task(Storage&& invocable) : m_Invocable(std::forward<decltype(invocable)>(invocable)) {};
	virtual ~task() = default;

	void operator()() override {
		std::invoke(m_Invocable);
	}

  private:
	Actual_Storage m_Invocable;
};
}	 // namespace psl::async::details
