#pragma once
#include <atomic>
#include <cassert>
#include <thread>

namespace psl {

/// \brief A thread safety guard that can be used to ensure that a certain scope is only accessed by a single thread at a time.
class thread_safety_guard_t {
  public:
	class scoped_guard_t {
		const thread_safety_guard_t& m_Guard;
		bool m_isPrimaryOwner {false};

	  public:
		explicit scoped_guard_t(const thread_safety_guard_t& guard) : m_Guard(guard) {
			auto current_id = std::this_thread::get_id();
			auto expected	= std::thread::id {};

			if(m_Guard.m_Owner.compare_exchange_strong(expected, current_id)) {
				m_isPrimaryOwner	= true;
				m_Guard.m_Recursion = 1;
			} else if(expected == current_id) {
				m_Guard.m_Recursion++;
			} else {
				assert(false && "Thread safety violation: scope owned by another thread!");
			}
		}

		~scoped_guard_t() {
			if(--m_Guard.m_Recursion == 0) {
				assert(m_isPrimaryOwner &&
					   "Thread safety violation: recursion count reached zero, but not primary owner!");
				m_Guard.m_Owner.store(std::thread::id {});
			}
		}

		scoped_guard_t(scoped_guard_t const&)			 = delete;
		scoped_guard_t& operator=(scoped_guard_t const&) = delete;
		scoped_guard_t(scoped_guard_t&&)				 = delete;
		scoped_guard_t& operator=(scoped_guard_t&&)		 = delete;
	};

	scoped_guard_t scoped_guard() const {
		return scoped_guard_t(*this);
	}

  private:
	mutable std::atomic<std::thread::id> m_Owner {};
	mutable std::atomic<size_t> m_Recursion {0};
};

namespace {
	class thread_safety_noop_guard_t {
	  public:
		class scoped_guard_t {
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
}	 // namespace

#if defined(PE_DEBUG)
using dbg_thread_safety_guard_t = thread_safety_guard_t;
#else
using dbg_thread_safety_guard_t = thread_safety_noop_guard_t;
#endif
}	 // namespace psl
