#pragma once
#include <atomic>
#include <thread>

namespace psl {

/// \brief A thread safety guard that can be used to ensure that a certain scope is only accessed by a single thread at a time.
class thread_safety_guard_t final {
  public:
	/// \brief A scoped guard that ensures that the current thread is the only one accessing the guarded scope.
	class scoped_guard_t final {
		const thread_safety_guard_t& m_Guard;
		bool m_isPrimaryOwner {false};

	  public:
		explicit scoped_guard_t(const thread_safety_guard_t& guard);
		~scoped_guard_t();

		scoped_guard_t(scoped_guard_t const&)			 = delete;
		scoped_guard_t& operator=(scoped_guard_t const&) = delete;
		scoped_guard_t(scoped_guard_t&&)				 = delete;
		scoped_guard_t& operator=(scoped_guard_t&&)		 = delete;
	};

	/// \brief Creates a new scoped guard that will lock the current thread to the guarded scope.
	scoped_guard_t scoped_guard() const {
		return scoped_guard_t(*this);
	}

  private:
	mutable std::atomic<std::thread::id> m_Owner {};
	mutable std::atomic<size_t> m_Recursion {0};
};

/// \brief A thread safety guard that does nothing. Mirrors the interface of thread_safety_guard_t.
class thread_safety_noop_guard_t final {
  public:
	class scoped_guard_t final {
	  public:
		explicit scoped_guard_t(const thread_safety_noop_guard_t&) {}
		~scoped_guard_t() {}
		scoped_guard_t(scoped_guard_t const&)			 = delete;
		scoped_guard_t& operator=(scoped_guard_t const&) = delete;
		scoped_guard_t(scoped_guard_t&&)				 = delete;
		scoped_guard_t& operator=(scoped_guard_t&&)		 = delete;
	};
	scoped_guard_t scoped_guard() const {
		return scoped_guard_t(*this);
	}
};

/// \brief Depending on the build configuration, this type alias will either be a real thread safety guard or a noop guard.
using dbg_thread_safety_guard_t =
#if defined(PE_DEBUG)
  thread_safety_guard_t;
#else
  thread_safety_noop_guard_t;
#endif
}	 // namespace psl
