#include "psl/memory/segment.hpp"
#include "psl/memory/range.hpp"

using namespace memory;
static memory::range_t def_range {0u, 0u};
segment::segment() : m_Range(&def_range), m_IsVirtual(true) {}
segment::segment(memory::range_t& _range, bool physically_backed) :
	m_Range(&_range), m_IsVirtual(!physically_backed) {

					  };

const range_t& segment::range() const noexcept { return *m_Range; }

bool segment::is_virtual() const { return m_IsVirtual; }

bool segment::is_valid() const { return m_Range != nullptr && m_Range != &def_range; }
