#pragma once
#include <type_traits>

namespace details {
template <typename F>
class monadic_container {
  public:
	monadic_container(F&& f) : callable(f) {};

	auto operator()() -> typename std::invoke_result_t<F> { return std::invoke(callable); }
	F callable;
};

class monadic_stub {
  public:
	monadic_stub(bool success = true) : m_Return(success) {}

	template <typename T>
	monadic_stub(T&& target) {
		call(std::forward<T>(target));
	}

	template <typename T, typename Comparator>
	monadic_stub(T&& target, Comparator&& comparator) {
		call(std::forward<T>(target), std::forward<Comparator>(comparator));
	}

	template <typename T>
	monadic_stub& then(T&& target) {
		if(m_Return)
			call(std::forward<T>(target));

		return *this;
	}


	template <typename T, typename Comparator>
	monadic_stub& then(T&& target, Comparator&& comparator) {
		if(m_Return)
			call(std::forward<T>(target), std::forward<Comparator>(comparator));

		return *this;
	}


	template <typename T0, typename T1>
	monadic_stub& then_or(T0&& target0, T1&& target1) {
		if(m_Return)
			call(std::forward<T0>(target0));
		else
			call(std::forward<T1>(target1));

		return *this;
	}
	template <typename T0, typename T1, typename Comparator>
	monadic_stub& then_or(T0&& target0, T1&& target1, Comparator&& comparator) {
		if(m_Return)
			call(std::forward<T0>(target0), std::forward<Comparator>(comparator));
		else
			call(std::forward<T1>(target1), std::forward<Comparator>(comparator));

		return *this;
	}
	template <typename T0, typename T1, typename Comparator0, typename Comparator1>
	monadic_stub& then_or(T0&& target0, T1&& target1, Comparator0&& comparator0, Comparator1&& comparator1) {
		if(m_Return)
			call(std::forward<T0>(target0), std::forward<Comparator0>(comparator0));
		else
			call(std::forward<T1>(target1), std::forward<Comparator1>(comparator1));

		return *this;
	}

	template <typename... Ts>
	monadic_stub& all(Ts&&... targets) {
		if(m_Return) {
			monadic_stub sub_stub {true};
			(sub_stub.then(std::forward<Ts>(targets)), ...);
			m_Return = sub_stub.m_Return;
		}
		return *this;
	}

	template <typename... Ts>
	monadic_stub& any(Ts&&... targets) {
		if(m_Return) {
			monadic_stub sub_stub {true};
			m_Return = false;
			(std::invoke([&]() {
				 sub_stub.call(std::forward<Ts>(targets));
				 m_Return |= sub_stub.success();
			 }),
			 ...);
		}
		return *this;
	}

	template <typename... Ts>
	monadic_stub& at_least(size_t count, Ts&&... targets) {
		if(m_Return) {
			monadic_stub sub_stub {true};
			size_t success_count {0};
			(std::invoke([&success_count, &sub_stub, &targets]() {
				 sub_stub.call(std::forward<Ts>(targets));
				 success_count = ((sub_stub.success()) ? success_count + 1 : success_count);
			 }),
			 ...);
			m_Return = success_count >= count;
		}
		return *this;
	}


	template <typename... Ts>
	monadic_stub& at_most(size_t count, Ts&&... targets) {
		if(m_Return) {
			monadic_stub sub_stub {true};
			size_t success_count {0};
			(std::invoke([&success_count, &sub_stub, &targets]() {
				 sub_stub.call(std::forward<Ts>(targets));
				 success_count = ((sub_stub.success()) ? success_count + 1 : success_count);
			 }),
			 ...);
			m_Return = success_count <= count;
		}
		return *this;
	}

	bool success() const noexcept { return m_Return; };

  private:
	template <typename T, typename Fn>
	void call(T&& target, Fn&& comparator) {
		if constexpr(std::is_same<typename std::invoke_result<T>::type, void>::value) {
			std::invoke(target);
			m_Return = true;
		} else {
			m_Return = std::invoke(comparator, std::invoke(target));
		}
	}

	template <typename T>
	void call(T&& target) {
		if constexpr(std::is_same<typename std::invoke_result<T>::type, void>::value) {
			std::invoke(target);
			m_Return = true;
		} else {
			m_Return = std::invoke(target);
		}
	}

	template <>
	void call(monadic_stub&& target) {
		m_Return = target.m_Return;
	}

	template <>
	void call(monadic_stub& target) {
		m_Return = target.m_Return;
	}

	template <>
	void call(const monadic_stub& target) {
		m_Return = target.m_Return;
	}

	bool m_Return {false};
};
}	 // namespace details

template <typename T>
static ::details::monadic_stub when(T&& target) {
	return ::details::monadic_stub(std::forward<T>(target));
}

template <typename T, typename Comparator>
static ::details::monadic_stub when(T&& target, Comparator&& comparator) {
	return ::details::monadic_stub(std::forward<T>(target), std::forward<Comparator>(comparator));
}

template <typename... Ts>
static ::details::monadic_stub when_any(Ts&&... targets) {
	::details::monadic_stub mon;
	return mon.any(std::forward<Ts>(targets)...);
}


template <typename... Ts>
static ::details::monadic_stub when_all(Ts&&... targets) {
	::details::monadic_stub mon;
	return mon.all(std::forward<Ts>(targets)...);
}

static ::details::monadic_stub monadic() {
	return ::details::monadic_stub {};
}
//
// template <class T>
// class monad
//{
//
//};
//
// template <typename T>
// struct then_functor final
//{
//	template <class Te>
//	monad<Te> operator()(monad<Te>& opt)
//	{
//		if(not opt)
//		{
//			return opt.condition();
//		}
//		return invoke(this->call, *opt);
//	}
//
//	template <class Te>
//	monad<Te> const operator()(result<Te> const& opt)
//	{
//		return std::invoke(m_Callable);
//	}
//
//	T m_Callable;
//};
