#pragma once
#include "template_utils.h"

namespace psl
{
	template <typename T>
	class unique_ptr;

	/// \brief a non-owning 'smart' pointer type
	///
	/// \details A wrapper object that can contain a pointer to either null, or any type.
	/// It will not do any operations on the pointer that would imply ownership (such as cleaning up).
	template <typename T>
	class view_ptr
	{
	  public:
		using element_type = T;
		using pointer	  = T*;
		using reference	= T&;

		constexpr view_ptr() noexcept = default;
		constexpr view_ptr(pointer value) noexcept : m_Value(value) {}

		template <typename T2>
		constexpr view_ptr(const psl::unique_ptr<T2>& value) noexcept : m_Value(&value.get())
		{}

		~view_ptr() = default;
		view_ptr(const view_ptr& other) : m_Value(other.m_Value){};
		view_ptr(view_ptr&& other) noexcept : m_Value(other.m_Value) { other.m_Value = nullptr; };
		view_ptr& operator=(const view_ptr& other)
		{
			if(this != &other)
			{
				m_Value = other.m_Value;
			}
			return *this;
		};
		view_ptr& operator=(view_ptr&& other) noexcept
		{
			if(this != &other)
			{
				m_Value		  = other.m_Value;
				other.m_Value = nullptr;
			}
			return *this;
		};

		constexpr pointer get() const noexcept { return m_Value; }

		constexpr reference operator*() const noexcept
		{
			assert_debug_break(m_Value != nullptr);
			return *m_Value;
		}

		constexpr pointer operator->() const noexcept { return m_Value; }

		constexpr explicit operator bool() const noexcept { return m_Value != nullptr; }
		constexpr operator pointer() const noexcept { return m_Value; }

		constexpr pointer release() noexcept
		{
			pointer p(m_Value);
			reset();
			return p;
		}

		constexpr void reset(pointer p = nullptr) noexcept { m_Value = p; }

		constexpr void swap(view_ptr& other) noexcept
		{
			using std::swap;
			swap(m_Value, other.m_Value);
		}

	  private:
		pointer m_Value{nullptr};
	};

	template <typename T1, typename T2>
	constexpr bool operator==(view_ptr<T1> p1, view_ptr<T2> p2) noexcept
	{
		return p1.get() == p2.get();
	}

	template <typename T1, typename T2>
	constexpr bool operator!=(view_ptr<T1> p1, view_ptr<T2> p2) noexcept
	{
		return !(p1 == p2);
	}

	template <typename T>
	constexpr bool operator==(view_ptr<T> p, std::nullptr_t) noexcept
	{
		return !p;
	}

	template <typename T>
	constexpr bool operator==(std::nullptr_t, view_ptr<T> p) noexcept
	{
		return !p;
	}

	template <typename T>
	constexpr bool operator!=(view_ptr<T> p, std::nullptr_t) noexcept
	{
		return static_cast<bool>(p);
	}

	template <typename T>
	constexpr bool operator!=(std::nullptr_t, view_ptr<T> p) noexcept
	{
		return static_cast<bool>(p);
	}
} // namespace psl

template<typename T>
constexpr static void swap(psl::view_ptr<T>& lhs, psl::view_ptr<T>& rhs) noexcept
{
	lhs.swap(rhs);
}

namespace std
{
	template <typename T>
	constexpr static void swap(psl::view_ptr<T>& lhs, psl::view_ptr<T>& rhs) noexcept
	{
		lhs.swap(rhs);
	}

	template <class T>
	struct hash<::psl::view_ptr<T>>
	{
		size_t operator()(::psl::view_ptr<T> p) const noexcept { return std::hash<T*>()(p.get()); }
	};

} // namespace std