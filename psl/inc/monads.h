#pragma once
#include <type_traits>
#include <any>

//namespace psl

	namespace details
	{
		class monadic_stub
		{
		public:
			monadic_stub(bool success = true) : m_Return(success)
			{}

			template<typename T>
			monadic_stub(T&& target)
			{
				call(std::forward<T>(target));
			}

			template<typename T>
			monadic_stub& then(T&& target)
			{
				if(m_Return)
					call(std::forward<T>(target));
				
				return *this;
			}

			template<typename T0, typename T1>
			monadic_stub& then_or(T0&& target0, T1&& target1)
			{
				if(m_Return)
					call(std::forward<T0>(target0));
				else
					call(std::forward<T1>(target1));

				return *this;
			}

			template<typename... Ts>
			monadic_stub& all(Ts&&... targets)
			{
				if(m_Return)
				{
					monadic_stub sub_stub{true};
					(sub_stub.then(std::forward<Ts>(targets)), ...);
					m_Return = sub_stub.m_Return;
				}
				return *this;
			}

			template<typename... Ts>
			monadic_stub& any(Ts&&... targets)
			{
				if(m_Return)
				{
					monadic_stub sub_stub{true};
					m_Return = false;
					(std::invoke([&](){sub_stub.call(std::forward<Ts>(targets)); m_Return |= sub_stub.success();}), ...);
				}
				return *this;
			}

			template<typename... Ts>
			monadic_stub& at_least(size_t count, Ts&&... targets)
			{
				if(m_Return)
				{
					monadic_stub sub_stub{true};
					size_t success_count {0};
					(std::invoke([&success_count, &sub_stub, &targets]() { sub_stub.call(std::forward<Ts>(targets)); success_count = ((sub_stub.success()) ? success_count + 1 : success_count); }), ...);
					m_Return = success_count >= count;
				}
				return *this;
			}


			template<typename... Ts>
			monadic_stub& at_most(size_t count, Ts&&... targets)
			{
				if(m_Return)
				{
					monadic_stub sub_stub{true};
					size_t success_count{0};
					(std::invoke([&success_count, &sub_stub, &targets]() { sub_stub.call(std::forward<Ts>(targets)); success_count = ((sub_stub.success()) ? success_count + 1 : success_count); }), ...);
					m_Return = success_count <= count;
				}
				return *this;
			}

			bool success() const noexcept { return m_Return; };

		private:

			template<typename T>
			void call(T&& target)
			{
				if constexpr(std::is_same<typename std::invoke_result<T>::type, void>::value)
				{
					std::invoke(target);
					m_Return = true;
				}
				else
				{
					m_Return = std::invoke(target);
				}
			}

			template<>
			void call(monadic_stub&& target)
			{
				m_Return = target.m_Return;
			}

			template<>
			void call(monadic_stub& target)
			{
				m_Return = target.m_Return;
			}

			template<>
			void call(const monadic_stub& target)
			{
				m_Return = target.m_Return;
			}

			bool m_Return{false};
		};
	}

	template<typename T>
	static ::details::monadic_stub when(T&& target)
	{
		return ::details::monadic_stub(std::forward<T>(target));
	}


	template<typename... Ts>
	static ::details::monadic_stub when_any(Ts&&... targets)
	{
		::details::monadic_stub mon;
		return mon.any(std::forward<Ts>(targets)...);
	}


	template<typename... Ts>
	static ::details::monadic_stub when_all(Ts&&... targets)
	{
		::details::monadic_stub mon;
		return mon.all(std::forward<Ts>(targets)...);
	}
