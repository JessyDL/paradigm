#pragma once
#include <system_error>

namespace psl
{
	template<typename T>
	class result
	{
	public:
		result(std::error_code error_) : m_Dummy(), m_Error(error_)
		{

		}

		result(T value_) : m_Value(value_), m_Error({})
		{
		}

		template<typename... Args>
		result(Args&&... args) : m_Value(std::forward<Args>(args)...), m_Error({})
		{};

		template<typename = typename std::enable_if<std::is_copy_constructible<T>::value>::type>
		result(const result& other) : m_Error(other.m_Error)
		{
			if(!m_Error)
				m_Value = other.m_Value;
		}

		template<typename = typename std::enable_if<std::is_copy_assignable<T>::value>::type>
		result& operator=(const result& other)
		{
			if(this != &other)
			{
				m_Error = other.error;
				if(!m_Error)
					m_Value = (other.m_Value);
			}
			return *this;
		}

		T& value()
		{
			if(m_Error)
				throw std::runtime_error("invalid access to result type. result type is false, but you tried accessing it anyhow");
			return m_Value;
		}

		const std::error_code& error() const
		{
			return m_Error;
		}

	private:
		struct dummy {};
		union
		{
			dummy m_Dummy;
			T m_Value;
		};
		std::error_code m_Error;
	};
}
