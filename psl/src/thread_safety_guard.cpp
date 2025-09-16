#include "psl/thread_safety_guard.hpp"
#include <cassert>
#include <cstdio>

namespace psl {
thread_safety_guard_t::scoped_guard_t::scoped_guard_t(const thread_safety_guard_t& guard) : m_Guard(guard) {
	auto current_id = std::this_thread::get_id();
	auto expected	= std::thread::id {};

	if(m_Guard.m_Owner.compare_exchange_strong(expected, current_id)) {
		m_isPrimaryOwner	= true;
		m_Guard.m_Recursion = 1;
	} else if(expected == current_id) {
		m_Guard.m_Recursion++;
	} else {
#ifndef NDEBUG
		assert(false && "Thread safety violation: scope already owned by another thread");
#else
		std::fprintf(stderr,
					 "Thread safety violation: scope owned by thread %zu, current thread is %zu\n",
					 std::hash<std::thread::id> {}(expected),
					 std::hash<std::thread::id> {}(current_id));
#endif
		std::abort();
	}
}

thread_safety_guard_t::scoped_guard_t::~scoped_guard_t() {
	if(--m_Guard.m_Recursion == 0) {
		if(!m_isPrimaryOwner) {
#ifndef NDEBUG
			assert(false && "Thread safety violation: recursion count reached zero, but not primary owner");
#else
			std::fprintf(stderr, "Thread safety violation: recursion count reached zero, but not primary owner!\n");
#endif
			std::abort();
		}
		m_Guard.m_Owner.store(std::thread::id {});
	}
}
}	 // namespace psl
