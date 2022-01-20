#pragma once
#include "delegate.hpp"
#include <cstring>	  // std::mem*
#include <functional>
#include <vector>

namespace internals
{
	namespace event
	{
		template <int N>
		struct placeholder
		{
			static placeholder ph;
		};

		template <int N>
		placeholder<N> placeholder<N>::ph;
		class AnyClass;
		using AnyPtrThis = AnyClass*;

		template <class TOut, class TIn>
		union HorribleUnion
		{
			TOut out;
			TIn in;
		};
		template <class TOut, class TIn>
		inline TOut horrible_cast(TIn mIn) noexcept
		{
			HorribleUnion<TOut, TIn> u;
			static_assert(sizeof(TIn) == sizeof(u) && sizeof(TIn) == sizeof(TOut), "Cannot use horrible_cast<>");
			u.in = mIn;
			return u.out;
		}
		template <class TOut, class TIn>
		inline TOut unsafe_horrible_cast(TIn mIn) noexcept
		{
			HorribleUnion<TOut, TIn> u;
			u.in = mIn;
			return u.out;
		}

		struct event_signature_t
		{
		  public:
			template <typename Function>
			event_signature_t(Function* func) noexcept
			{
				ptrFunction = horrible_cast<AnyPtrThis>(func);
			}

			template <typename Function, typename Class>
			event_signature_t(Class* mThis, Function Class::*func) noexcept
			{
				ptrThis		= (mThis);
				ptrFunction = unsafe_horrible_cast<AnyPtrThis>(func);
			}

			inline bool operator==(std::nullptr_t) const noexcept
			{
				return ptrThis == nullptr && ptrFunction == nullptr;
			}
			inline bool operator==(const event_signature_t& mRhs) const noexcept
			{
				return ptrThis == mRhs.ptrThis && ptrFunction == mRhs.ptrFunction;
			}
			inline bool operator!=(std::nullptr_t) const noexcept { return !operator==(nullptr); }
			inline bool operator!=(const event_signature_t& mRhs) const noexcept { return !operator==(mRhs); }
			inline bool operator<(const event_signature_t& mRhs) const
			{
				return ptrThis != mRhs.ptrThis ? ptrThis < mRhs.ptrThis
											   : std::memcmp(&ptrFunction, &mRhs.ptrFunction, sizeof(ptrFunction)) < 0;
			}
			inline bool operator>(const event_signature_t& mRhs) const { return !operator<(mRhs); }

			event_signature_t(const event_signature_t& t) = default;
			event_signature_t(event_signature_t&& t)	  = default;
			event_signature_t& operator=(const event_signature_t& t) = default;	   // disable assignment

		  private:
			void* ptrThis {nullptr};
			AnyPtrThis ptrFunction {nullptr};
		};
	}	 // namespace event
}	 // namespace internals

namespace common
{
	template <typename... T>
	class event_listener_t
	{
	  protected:
		struct internal_event_t
		{
			typedef psl::delegate<void(T...)> functype;

			internal_event_t(const internals::event::event_signature_t& signature) : signature(signature) {};

			// internal_event_t(internals::event::event_signature_t signature, functype func) : signature(signature),
			// function(func) {};
			internal_event_t(const internals::event::event_signature_t& signature, functype&& func) :
				signature(signature), function(std::forward<functype>(func)) {};

			template <typename Function, typename Class>
			void SetFunction(Class* mThis, Function&& func)
			{
				function = (std::bind(func, mThis));
			};
			internals::event::event_signature_t signature;
			functype function;
		};

	  public:
		event_listener_t()						  = default;
		virtual ~event_listener_t()				  = default;
		event_listener_t(const event_listener_t&) = default;			   // Copy constructor
		event_listener_t(event_listener_t&&)	  = default;			   // Move constructor
		event_listener_t& operator=(const event_listener_t&) = default;	   // Copy assignment operator
		event_listener_t& operator=(event_listener_t&&) = default;		   // Move assignment operator


		template <typename Function>
		void Subscribe(Function&& func)
		{
			InternalSubscribe(std::forward<Function>(func), std::make_integer_sequence<int, sizeof...(T)>());
		}

		template <typename Function, typename Class>
		void Subscribe(Class* mThis, Function&& func)
		{
			InternalSubscribe(mThis, std::forward<Function>(func), std::make_integer_sequence<int, sizeof...(T)>());
		}

		template <typename Function>
		inline void Unsubscribe(Function&& func)
		{
			internals::event::event_signature_t sig(std::forward<Function>(func));
			for(auto it = m_Listeners.begin(); it != m_Listeners.end(); ++it)
			{
				if((*it).signature == sig)
				{
					m_Listeners.erase(it);
					break;
				}
			}
		}

		template <typename Function, typename Class>
		inline void Unsubscribe(Class* mThis, Function Class::*func)
		{
			internals::event::event_signature_t sig(mThis, func);
			for(auto it = m_Listeners.begin(); it != m_Listeners.end(); ++it)
			{
				if((*it).signature == sig)
				{
					m_Listeners.erase(it);
					break;
				}
			}
		}

		template <typename Function, typename Class>
		event_listener_t& operator+=(std::pair<Class*, Function Class::*> pair)
		{
			Subscribe(pair.first, pair.second);
			return *this;
		}

		template <typename Function>
		event_listener_t& operator+=(Function* func)
		{
			Subscribe(func);
			return *this;
		}

		template <typename Function, typename Class>
		event_listener_t& operator-=(std::pair<Class*, Function Class::*> pair)
		{
			Unsubscribe(pair.first, pair.second);
			return *this;
		}

		template <typename Function>
		event_listener_t& operator-=(Function* func)
		{
			Unsubscribe(func);
			return *this;
		}

	  protected:
		std::vector<internal_event_t> m_Listeners;

	  private:
		template <typename Function, int... indices>
		void InternalSubscribe(Function&& func, std::integer_sequence<int, indices...>)
		{
			m_Listeners.emplace_back(std::move(internals::event::event_signature_t(func)),
									 std::move(std::bind(func, internals::event::placeholder<indices + 1>::ph...)));
		}

		template <typename Function, typename Class, int... indices>
		void InternalSubscribe(Class* mThis, Function&& func, std::integer_sequence<int, indices...>)
		{
			m_Listeners.emplace_back(
			  std::move(internals::event::event_signature_t(mThis, func)),
			  std::move(std::bind(func, mThis, internals::event::placeholder<indices + 1>::ph...)));
		}
	};

	template <>
	class event_listener_t<void>
	{
	  protected:
		struct internal_event_t
		{
			typedef psl::delegate<void()> functype;

			internal_event_t(const internals::event::event_signature_t& signature) : signature(signature) {};

			internal_event_t(const internals::event::event_signature_t& signature, functype&& func) :
				signature(signature), function(std::forward<functype>(func)) {};

			template <typename Function, typename Class>
			void SetFunction(Class* mThis, Function&& func)
			{
				function = (std::bind(func, mThis));
			};
			internals::event::event_signature_t signature;
			functype function;
		};

	  public:
		event_listener_t()						  = default;
		virtual ~event_listener_t()				  = default;
		event_listener_t(const event_listener_t&) = default;			   // Copy constructor
		event_listener_t(event_listener_t&&)	  = default;			   // Move constructor
		event_listener_t& operator=(const event_listener_t&) = default;	   // Copy assignment operator
		event_listener_t& operator=(event_listener_t&&) = default;		   // Move assignment operator

		template <typename Function>
		void Subscribe(Function&& func)
		{
			m_Listeners.emplace_back(std::move(internals::event::event_signature_t(func)), std::move(std::bind(func)));
		}

		template <typename Function, typename Class>
		void Subscribe(Class* mThis, Function&& func)
		{
			m_Listeners.emplace_back(std::move(internals::event::event_signature_t(mThis, func)),
									 std::move(std::bind(func, mThis)));
		}

		template <typename Function>
		inline void Unsubscribe(Function* func)
		{
			internals::event::event_signature_t sig(func);
			for(auto it = m_Listeners.begin(); it != m_Listeners.end(); ++it)
			{
				if((*it).signature == sig)
				{
					m_Listeners.erase(it);
					break;
				}
			}
		}

		template <typename Function, typename Class>
		inline void Unsubscribe(Class* mThis, Function Class::*func)
		{
			internals::event::event_signature_t sig(mThis, func);
			for(auto it = m_Listeners.begin(); it != m_Listeners.end(); ++it)
			{
				if((*it).signature == sig)
				{
					m_Listeners.erase(it);
					break;
				}
			}
		}

		template <typename Function, typename Class>
		event_listener_t& operator+=(std::pair<Class*, Function Class::*> pair)
		{
			Subscribe(pair.first, pair.second);
			return *this;
		}

		template <typename Function>
		event_listener_t& operator+=(Function* func)
		{
			Subscribe(func);
			return *this;
		}

		template <typename Function, typename Class>
		event_listener_t& operator-=(std::pair<Class*, Function Class::*> pair)
		{
			Unsubscribe(pair.first, pair.second);
			return *this;
		}

		template <typename Function>
		event_listener_t& operator-=(Function* func)
		{
			Unsubscribe(func);
			return *this;
		}

	  protected:
		std::vector<internal_event_t> m_Listeners;
	};

	template <typename... T>
	class event final : public event_listener_t<T...>
	{
	  public:
		event_listener_t<T...>& Listener() const { return (event_listener_t<T...>)(*this); }

		void operator()(T... arguments)
		{
			auto listenerCopy = event_listener_t<T...>::m_Listeners;
			for(auto it = listenerCopy.begin(); it != listenerCopy.end(); ++it)
			{
				(*it).function(arguments...);
				//(*it).function();
			}
		}

		void Execute(T... arguments)
		{
			auto listenerCopy = event_listener_t<T...>::m_Listeners;
			for(auto it = listenerCopy.begin(); it != listenerCopy.end(); ++it)
			{
				(*it).function(arguments...);
			}
		}

		/*void Execute(T&&... arguments)
		{
			this->operator()(std::forward<T>(arguments)...);
		}*/
	};

	template <>
	class event<void> final : public event_listener_t<void>
	{
	  public:
		event_listener_t& Listener() { return (*this); }

		void operator()()
		{
			auto listenerCopy = event_listener_t<void>::m_Listeners;
			for(auto it = listenerCopy.begin(); it != listenerCopy.end(); ++it)
			{
				(*it).function();
			}
		}

		void Execute() { this->operator()(); }
	};

	// template<typename ... T>
	// event_listener_t<T...>  eventListener<T...>;
}	 // namespace common


namespace std
{
	template <int N>
	struct is_placeholder<internals::event::placeholder<N>> : integral_constant<int, N>
	{};
}	 // namespace std
