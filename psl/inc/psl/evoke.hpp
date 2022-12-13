#pragma once
#include "delegate.hpp"
#include <array>
#include <functional>

/// Class: psl::evocable
/// Info:	evocables are similar to common::event in that you can invoke a function or method at any time.
///			Principle difference is that with evocables you assign the std::function's parameters beforehand
///			when you invoke the function/method, you use the assigned parameters from beforehand.
/// Details:	Precautions should be taken when sending std::ref() and pointers to the evocable as to not
///				Invoke functions/methods after their lifetime has passed.
///				There are 3 duck typed versions:
///					- evocable: you can only invoke on this variation
///					- evocableR: the invokes return value is known
///					- evocableT: mostly for completion and consistency with the Event API, has very few use cases

namespace utility::templates
{
enum class type_qualifier_value : uint8_t
{
	CONST_MOD = 1 << 0,
	ATOMIC	  = 1 << 1,
	VALUE	  = 1 << 2,
	REFERENCE = 1 << 3
};

enum class type_qualifier : uint8_t
{
	IS_POINTER		   = !(uint8_t)type_qualifier_value::VALUE | (uint8_t)type_qualifier_value::REFERENCE,
	IS_VALUE_REFERENCE = (uint8_t)type_qualifier_value::VALUE | (uint8_t)type_qualifier_value::REFERENCE,
	IS_VALUE		   = (uint8_t)type_qualifier_value::VALUE | !(uint8_t)type_qualifier_value::REFERENCE,

	IS_CONST_POINTER = (uint8_t)type_qualifier_value::CONST_MOD | !(uint8_t)type_qualifier_value::VALUE |
					   (uint8_t)type_qualifier_value::REFERENCE,
	IS_CONST_VALUE_REFERENCE = (uint8_t)type_qualifier_value::CONST_MOD | (uint8_t)type_qualifier_value::VALUE |
							   (uint8_t)type_qualifier_value::REFERENCE,
	IS_CONST_VALUE = (uint8_t)type_qualifier_value::CONST_MOD | (uint8_t)type_qualifier_value::VALUE |
					 !(uint8_t)type_qualifier_value::REFERENCE
};

template <typename T>
struct get_type_qualifier
{
	static constexpr type_qualifier value {type_qualifier::IS_VALUE};
};

template <typename T>
struct get_type_qualifier<T*>
{
	static constexpr type_qualifier value {type_qualifier::IS_POINTER};
};


template <typename T>
struct get_type_qualifier<T const*>
{
	static constexpr type_qualifier value {type_qualifier::IS_CONST_POINTER};
};


template <typename T>
struct get_type_qualifier<T const* const>
{
	static constexpr type_qualifier value {type_qualifier::IS_POINTER};
};

template <typename T>
struct get_type_qualifier<T* const>
{
	static constexpr type_qualifier value {type_qualifier::IS_POINTER};
};

template <typename T>
struct get_type_qualifier<T**>
{
	static constexpr type_qualifier value {type_qualifier::IS_POINTER};
};

template <typename T>
struct get_type_qualifier<T&>
{
	static constexpr type_qualifier value {type_qualifier::IS_VALUE_REFERENCE};
};


template <typename T>
struct get_type_qualifier<const T&>
{
	static constexpr type_qualifier value {type_qualifier::IS_CONST_VALUE_REFERENCE};
};

template <typename T>
struct get_type_qualifier<T&&>
{
	static constexpr type_qualifier value {type_qualifier::IS_VALUE};
};

template <typename... T>
struct get_type_qualifier_v
{
	static constexpr std::array<type_qualifier, sizeof...(T)> value {
	  utility::templates::get_type_qualifier<T> {}.value...};
};

template <typename... T>
struct get_type_qualifier_v<std::tuple<T...>>
{
	static constexpr std::array<type_qualifier, sizeof...(T)> value {
	  utility::templates::get_type_qualifier<T> {}.value...};
};
}	 // namespace utility::templates
namespace Common
{
namespace details
{
	template <typename T>
	struct transform_moveable
	{
		using type = T;
	};

	template <typename T>
	struct transform_moveable<T&&>
	{
		using type = T;
	};

	template <typename... T>
	struct transform_moveables
	{
		using tuple_type = std::tuple<typename transform_moveable<T>::type...>;
	};
}	 // namespace details
// Dummy base for duck typing reasons.
class evocable
{
  protected:
	evocable()							   = default;
	evocable(const evocable&)			   = default;	 // Copy constructor
	evocable(evocable&&)				   = default;	 // Move constructor
	evocable& operator=(const evocable&) & = default;	 // Copy assignment operator
	evocable& operator=(evocable&&) &	   = default;	 // Move assignment operator


  public:
	void operator()() { return; }
	virtual ~evocable() {}	  // Destructor
};

template <typename R>
class evocableR : public virtual evocable
{
  public:
	virtual R operator()() = 0;
	virtual ~evocableR() {};
};

template <typename... T>
class evocableT : public virtual evocable
{
  public:
	virtual ~evocableT() {};
};

// Return value (R) and multi param (T...) method and function signatures: ex. R function(T...) {};
template <typename R, typename... T>
class evoke final : public virtual evocableR<R>, public virtual evocableT<T...>
{
  public:
	using tuple_type	  = typename details::transform_moveables<T...>::tuple_type;
	using parameter_types = typename std::tuple<T...>;
	using return_type	  = R;

	evoke(std::function<R(T...)>&& f, T... args) :
		bind_(std::forward<std::function<R(T...)>>(f)), params(std::forward<T>(args)...)
	{}

	~evoke() {};

	R operator()() override { return execute_fn(std::index_sequence_for<T...> {}); }

	evoke(const evoke& e) : bind_(e.bind_), params(e.params) {};					 // Copy constructor
	evoke(evoke&& e) : bind_(std::move(e.bind_)), params(std::move(e.params)) {};	 // Move constructor
	evoke& operator=(const evoke& e) &												 // Copy assignment operator
	{
		if(this == &e)
		{
			bind_  = e.bind_;
			params = e.params;
		}
		return *this;
	};
	evoke& operator=(evoke&& e) &	 // Move assignment operator
	{
		if(this == &e)
		{
			bind_  = std::move(e.bind_);
			params = std::move(e.params);
		}
		return *this;
	};

	std::array<std::pair<std::uintptr_t, size_t>, sizeof...(T)> _parameter_locations() const
	{
		return parameter_loc(std::index_sequence_for<T...> {});
	}

  private:
	template <std::size_t... S>
	R execute_fn(std::index_sequence<S...>)
	{
		return bind_(std::forward<T>(std::get<S>(params))...);
	}

	template <std::size_t... S>
	std::array<std::pair<std::uintptr_t, size_t>, sizeof...(T)> parameter_loc(std::index_sequence<S...>) const
	{
		std::array<std::pair<std::uintptr_t, size_t>, sizeof...(T)> result {
		  std::make_pair((std::uintptr_t)&std::get<S>(params), sizeof(T))...};

		return result;
	}

	std::function<R(T...)> bind_;
	tuple_type params;
};


// Return value (R) and no parameters method and function signatures: ex. R function() {};
template <typename R>
class evoke<R, void> final : public virtual evocableR<R>, public virtual evocableT<void>
{
  public:
	using tuple_type	  = typename std::tuple<void>;
	using parameter_types = tuple_type;
	using return_type	  = R;

	evoke(std::function<R()>&& f) : bind_([f {std::forward<std::function<R()>>(f)}]() { return (f)(); }) {}
	~evoke() {};

	R operator()() override { return bind_(); }
	evoke(const evoke& e) : bind_(e.bind_) {};			// Copy constructor
	evoke(evoke&& e) : bind_(std::move(e.bind_)) {};	// Move constructor
	evoke& operator=(const evoke& e) &					// Copy assignment operator
	{
		if(this == &e)
		{
			bind_ = e.bind_;
		}
		return *this;
	};
	evoke& operator=(evoke&& e) &	 // Move assignment operator
	{
		if(this == &e)
		{
			bind_ = std::move(e.bind_);
		}
		return *this;
	};

	std::array<std::pair<std::uintptr_t, size_t>, 0> _parameter_locations() const
	{
		return std::array<std::pair<std::uintptr_t, size_t>, 0> {};
	}

  private:
	std::function<R()> bind_;
};


// No return value and multi param (T...) method and function signatures: ex. void function(T...) {};
template <typename... T>
class evoke<void, T...> final : public virtual evocableR<void>, public virtual evocableT<T...>
{
  public:
	using tuple_type	  = typename details::transform_moveables<T...>::tuple_type;
	using parameter_types = typename std::tuple<T...>;
	using return_type	  = void;

	template <typename... AT>
	evoke(std::function<void(T...)>&& f, AT&&... args) :
		bind_(std::forward<std::function<void(T...)>>(f)), params(std::forward<T>(args)...)
	{}
	~evoke() {};

	void operator()() override { execute_fn(std::index_sequence_for<T...> {}); }
	evoke(const evoke& e) : bind_(e.bind_), params(e.params) {};					 // Copy constructor
	evoke(evoke&& e) : bind_(std::move(e.bind_)), params(std::move(e.params)) {};	 // Move constructor
	evoke& operator=(const evoke& e) &												 // Copy assignment operator
	{
		if(this == &e)
		{
			bind_  = e.bind_;
			params = e.params;
		}
		return *this;
	};
	evoke& operator=(evoke&& e) &	 // Move assignment operator
	{
		if(this == &e)
		{
			bind_  = std::move(e.bind_);
			params = std::move(e.params);
		}
		return *this;
	};

	std::array<std::pair<std::uintptr_t, size_t>, sizeof...(T)> _parameter_locations() const
	{
		return parameter_loc(std::index_sequence_for<T...> {});
	}

  private:
	template <std::size_t... S>
	void execute_fn(std::index_sequence<S...>)
	{
		bind_(std::forward<T>(std::get<S>(params))...);
	}

	template <std::size_t... S>
	std::array<std::pair<std::uintptr_t, size_t>, sizeof...(T)> parameter_loc(std::index_sequence<S...>) const
	{
		std::array<std::pair<std::uintptr_t, size_t>, sizeof...(T)> result {
		  std::make_pair((std::uintptr_t)&std::get<S>(params), sizeof(T))...};
		return result;
	}

	std::function<void(T...)> bind_;
	tuple_type params;
};


// No return value and no parameters method and function signatures: ex. void function() {};
template <>
class evoke<void, void> final : public virtual evocableR<void>, public virtual evocableT<void>
{
  public:
	using tuple_type	  = typename std::tuple<void>;
	using parameter_types = tuple_type;
	using return_type	  = void;

	evoke(std::function<void()>&& f) : bind_([f {std::forward<std::function<void()>>(f)}]() { (f)(); }) {}
	~evoke() {};

	void operator()() override { bind_(); }
	evoke(const evoke& e) : bind_(e.bind_) {};			// Copy constructor
	evoke(evoke&& e) : bind_(std::move(e.bind_)) {};	// Move constructor
	evoke& operator=(const evoke& e) &					// Copy assignment operator
	{
		if(this == &e)
		{
			bind_ = e.bind_;
		}
		return *this;
	};
	evoke& operator=(evoke&& e) &	 // Move assignment operator
	{
		if(this == &e)
		{
			bind_ = std::move(e.bind_);
		}
		return *this;
	};

	std::array<std::pair<std::uintptr_t, size_t>, 0> _parameter_locations() const
	{
		return std::array<std::pair<std::uintptr_t, size_t>, 0> {};
	}

  private:
	std::function<void()> bind_;
};


/// Helper functions for compilers below C++17
/// The following functions assist in inferring the types you want

template <typename R, typename... T>
static psl::evoke<std::result_of_t<R(T...)>, T...> make_evocable(R&& r, T&&... parameters)
{
	return psl::evoke<std::result_of_t<R(T...)>, T...>(std::forward<R>(r), std::forward<T>(parameters)...);
}


template <typename R>
static psl::evoke<std::result_of_t<R()>, void> make_evocable(R&& r)
{
	return psl::evoke<std::result_of_t<R()>, void>(std::forward<R>(r));
}


template <typename... T>
static psl::evoke<void, T...> make_evocable(std::function<void(T...)>&& r, T&&... parameters)
{
	return psl::evoke<void, T...>(std::forward<std::function<void(T...)>>(r), std::forward<T>(parameters)...);
}

template <>
psl::evoke<void, void> make_evocable(std::function<void()>&& r)
{
	return psl::evoke<void, void>(std::forward<std::function<void()>>(r));
}


template <typename R, typename... T>
static psl::evoke<std::result_of_t<R(T...)>, T...>* make_evocable_ptr(R&& r, T&&... parameters)
{
	return new psl::evoke<std::result_of_t<R(T...)>, T...>(std::forward<R>(r), std::forward<T>(parameters)...);
}


template <typename R>
static psl::evoke<std::result_of_t<R()>, void>* make_evocable_ptr(R&& r)
{
	return new psl::evoke<std::result_of_t<R()>, void>(std::forward<R>(r));
}


template <typename... T>
static psl::evoke<void, T...>* make_evocable_ptr(std::function<void(T...)>&& r, T&&... parameters)
{
	return new psl::evoke<void, T...>(std::forward<std::function<void(T...)>>(r), std::forward<T>(parameters)...);
}

template <>
psl::evoke<void, void>* make_evocable_ptr(std::function<void()>&& r)
{
	return new psl::evoke<void, void>(std::forward<std::function<void()>>(r));
}

template <typename R, typename... T>
static psl::evocableR<std::result_of_t<R(T...)>>* make_evocable_r_ptr(R&& r, T&&... parameters)
{
	return new psl::evoke<std::result_of_t<R(T...)>, T...>(std::forward<R>(r), std::forward<T>(parameters)...);
}


template <typename R>
static psl::evocableR<std::result_of_t<R()>>* make_evocable_r_ptr(R&& r)
{
	return new psl::evoke<std::result_of_t<R()>, void>(std::forward<R>(r));
}


template <typename... T>
static psl::evocableR<void>* make_evocable_r_ptr(std::function<void(T...)>&& r, T&&... parameters)
{
	return new psl::evoke<void, T...>(std::forward<std::function<void(T...)>>(r), std::forward<T>(parameters)...);
}

template <>
psl::evocableR<void>* make_evocable_r_ptr(std::function<void()>&& r)
{
	return new psl::evoke<void, void>(std::forward<std::function<void()>>(r));
}
}	 // namespace Common
