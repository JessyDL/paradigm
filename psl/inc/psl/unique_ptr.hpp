#pragma once

namespace psl
{
template <typename T>
class unique_ptr
{
  public:
	unique_ptr() = default;
	unique_ptr(T* ptr) : m_Ptr(ptr) {};
	~unique_ptr() { delete(m_Ptr); }

	unique_ptr(const unique_ptr&)			 = delete;
	unique_ptr& operator=(const unique_ptr&) = delete;


	unique_ptr(unique_ptr&& other) noexcept : m_Ptr(other.m_Ptr) { other.m_Ptr = nullptr; };
	unique_ptr& operator=(unique_ptr&& other)
	{
		if(this != &other)
		{
			m_Ptr		= other.m_Ptr;
			other.m_Ptr = nullptr;
		}
		return *this;
	}

	T& get() const noexcept { return *m_Ptr; }

	T* operator->() noexcept { return m_Ptr; }

	T const* operator->() const noexcept { return m_Ptr; }

	T& operator*() noexcept { return *m_Ptr; }

	const T& operator*() const noexcept { return *m_Ptr; }

	unique_ptr<T>& operator=(T* other) noexcept
	{
		delete(m_Ptr);
		m_Ptr = other;
		return *this;
	}

	void reset() noexcept
	{
		delete(m_Ptr);
		m_Ptr = nullptr;
	}

	void release() noexcept { m_Ptr = nullptr; }

	operator bool() const noexcept { return m_Ptr != nullptr; }

  private:
	T* m_Ptr {nullptr};
};
}	 // namespace psl