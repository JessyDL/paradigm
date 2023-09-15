#include "psl/memory/region.hpp"
#include "psl/platform_def.hpp"
#include <algorithm>
#if defined(PE_PLATFORM_WINDOWS)
	#include <Windows.h>
	#define USE_WIN32
#elif defined(PE_PLATFORM_LINUX) || defined(PE_PLATFORM_ANDROID) || defined(PE_PLATFORM_MACOS)
	#include <sys/mman.h>
	#include <unistd.h>
	#define USE_POSIX
#else
	#define USE_GENERIC
#endif
#include "psl/assertions.hpp"
#include "psl/logging.hpp"
using namespace memory;

region::region(region& parent, memory::segment& segment, size_t pageSize, size_t alignment, allocator_base* allocator)
	: m_Parent(&parent), m_Allocator(allocator), m_Alignment(alignment) {
	m_PageSize = pageSize;
	m_Size	   = segment.range().size();
	m_Base	   = (void*)segment.range().begin;

#if defined(USE_WIN32)
	if(m_Allocator->is_physically_backed()) {
		m_PageState.resize(m_Size / pageSize);
	}
#endif
	m_Parent->m_Children.push_back(this);
	allocator->m_Region = this;
	allocator->initialize(this);
}

region::region(size_t size, size_t alignment, allocator_base* allocator)
	: m_Allocator(allocator), m_Alignment(alignment) {
	if(!allocator->is_physically_backed()) {
		m_PageSize = 0u;
		m_Size	   = size;
		m_Base	   = nullptr;
	} else {
#if defined(USE_WIN32)
		// DWORD dwPages = 0;              // Count of pages gotten so far

		SYSTEM_INFO sSysInfo;		 // Useful information about the system
		GetSystemInfo(&sSysInfo);	 // Initialize the structure.
		m_PageSize = sSysInfo.dwPageSize;

		m_Size = (size + m_PageSize - 1) / m_PageSize * m_PageSize;
		m_Base = VirtualAlloc(NULL,				 // System selects address
							  m_Size,			 // Size of allocation
							  MEM_RESERVE,		 // Allocate reserved pages
							  PAGE_NOACCESS);	 // Protection = no access

		m_PageState.resize(m_Size / m_PageSize);

		if(m_Base == nullptr) {
			// todo error state
			__debugbreak();
		}
#elif defined(USE_POSIX)
		m_PageSize = sysconf(_SC_PAGE_SIZE);

		m_Size = (size + m_PageSize - 1) / m_PageSize * m_PageSize;

		auto addr = mmap(NULL, m_Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if(addr == MAP_FAILED) {
			exit(EXIT_FAILURE);
		}
		m_Base = (unsigned char*)addr;

#endif
	}
	allocator->m_Region = this;
	allocator->initialize(this);
}

region::region(region&& other)
	: m_Allocator(other.m_Allocator), m_Alignment(other.m_Alignment), m_Base(other.m_Base), m_Size(other.m_Size),
#if defined(USE_WIN32)
	  m_PageState(std::move(other.m_PageState)),
#endif
	  m_Children(std::move(other.m_Children)), m_Parent(other.m_Parent), m_PageSize(other.m_PageSize) {
	if(m_Parent) {
		std::replace_if(
		  std::begin(m_Parent->m_Children),
		  std::end(m_Parent->m_Children),
		  [&other](auto& it) { return it == &other; },
		  this);
	}

	for(auto& it : m_Children) {
		it->m_Parent = this;
	}
	m_Allocator->m_Region = this;
	other.m_Allocator	  = nullptr;
	other.m_Base		  = nullptr;
	other.m_Parent		  = nullptr;
	other.m_Children.clear();
}

region& region::operator=(region&& other) {
	if(this != &other) {
		m_Allocator = other.m_Allocator;
		m_Alignment = other.m_Alignment;
		m_Base		= other.m_Base;
		m_Size		= other.m_Size;
#if defined(USE_WIN32)
		m_PageState = std::move(other.m_PageState);
#endif
		m_Children = std::move(other.m_Children);
		m_Parent   = other.m_Parent;
		m_PageSize = other.m_PageSize;

		std::replace_if(
		  std::begin(m_Parent->m_Children),
		  std::end(m_Parent->m_Children),
		  [&other](auto& it) { return it == &other; },
		  this);

		for(auto& it : m_Children) {
			it->m_Parent = this;
		}
		m_Allocator->m_Region = this;
		other.m_Allocator	  = nullptr;
		other.m_Base		  = nullptr;
		other.m_Parent		  = nullptr;
	}
	return *this;
}

std::optional<segment> region::allocate(size_t size) {
	return m_Allocator->allocate(size);
}

region::~region() {
	if(m_Children.size() != 0)
		debug_break();	  // todo: we need to figure out a good error here, children need to be cleared before their
						  // parents

	if(m_Parent != nullptr) {
		m_Parent->erase_region(*this);
		delete(m_Allocator);
	} else {
		if(m_Base == nullptr || !m_Allocator->is_physically_backed()) {
			if(m_Allocator)
				delete(m_Allocator);

			return;
		}
		delete(m_Allocator);
#if defined(USE_WIN32)
		VirtualFree(m_Base,			 // Base address of block
					0,				 // Bytes of committed pages
					MEM_RELEASE);	 // Decommit the pages
#elif defined(USE_POSIX)
		if(munmap(m_Base, sizeof(int)) == -1) {
			LOG_ERROR("munmap()() failed");
			exit(EXIT_FAILURE);
		}
#endif
	}
}

std::optional<memory::region>
region::create_region(size_t size, std::optional<size_t> alignment, allocator_base* allocator) {
	if(allocator->is_physically_backed() != m_Allocator->is_physically_backed()) {
		return {};
	}

#if defined(USE_WIN32)
	auto final_size = size;
	if(m_Allocator->is_physically_backed()) {
		size_t pages = 0u;
		pages		 = (size + (size % m_PageSize)) / m_PageSize;
		pages		 = (pages == 0) ? 1 : pages;
		final_size	 = pages * m_PageSize;
	}

	// we cheat and trick the allocator to allocate in page sized allocations
	auto cachedAlignment = m_Alignment;
	m_Alignment			 = m_PageSize;
	auto res			 = m_Allocator->allocate(final_size);
	m_Alignment			 = cachedAlignment;

	if(res) {
		size_t start = (res.value().range().begin - (std::uintptr_t)(m_Base)) / m_PageSize;
		size_t end	 = (res.value().range().end - (std::uintptr_t)(m_Base)) / m_PageSize;
		std::fill(std::begin(m_PageState) + start, std::begin(m_PageState) + end, state::DEFERRED);

		return memory::region {*this, res.value(), m_PageSize, alignment.value_or(m_Alignment), allocator};
	}

#elif defined(USE_POSIX)
	// we cheat and trick the allocator to allocate in page sized allocations
	auto cachedAlignment = m_Alignment;
	m_Alignment = m_PageSize;
	auto res = m_Allocator->allocate(size);
	m_Alignment = cachedAlignment;
	if(res) {
		return memory::region {*this, res.value(), 0, alignment.value_or(m_Alignment), allocator};
	}
#endif

	return {};
}

bool region::erase_region(memory::region& child) {
	auto it = std::find_if(
	  std::begin(m_Children), std::end(m_Children), [&child](const memory::region* item) { return item == &child; });

	if(it == std::end(m_Children))
		return false;

	memory::region* childPtr = *it;

	m_Children.erase(it);

	auto range =
	  memory::range_t {(std::uintptr_t)(childPtr->data()), (std::uintptr_t)(childPtr->data()) + childPtr->size()};
	memory::segment segm {range, child.m_Allocator->is_physically_backed()};
#if defined(USE_WIN32)
	size_t start = (range.begin - (std::uintptr_t)(m_Base)) / m_PageSize;
	size_t end	 = (range.end - (std::uintptr_t)(m_Base)) / m_PageSize;
	std::fill(std::begin(m_PageState) + start, std::begin(m_PageState) + end, state::COMMITED);
#endif
	return deallocate(segm);
}

bool region::commit(const memory::range_t& range) {
	if(!m_Allocator->is_physically_backed()) {
		return true;
	}
	// we only keep track on windows what the pages are
#if defined(USE_WIN32)
	auto pages = page_range(range);
	auto start = pages.first;
	for(auto i = pages.first; i < pages.second; ++i) {
		if(m_PageState[i] != state::COMMITED) {
			continue;
		} else {
			if(start != i) {
				auto loc = (std::uintptr_t)(m_Base) + (m_PageSize * start);

				auto lpvResult = VirtualAlloc((void*)loc,				   // Next page to commit
											  m_PageSize * (i - start),	   // Page size, in bytes
											  MEM_COMMIT,				   // Allocate a committed page
											  PAGE_READWRITE);			   // Read/write access

				if(lpvResult != NULL) {
					auto end =
					  (pages.second > m_PageState.size()) ? std::end(m_PageState) : std::begin(m_PageState) + i;
					std::fill(std::begin(m_PageState) + start, end, state::COMMITED);
				} else
					return false;
			}
			start = i + 1;
		}
	}

	if(start != pages.second) {
		auto loc = (std::uintptr_t)(m_Base) + (m_PageSize * start);

		auto lpvResult = VirtualAlloc((void*)loc,							  // Next page to commit
									  m_PageSize * (pages.second - start),	  // Page size, in bytes
									  MEM_COMMIT,							  // Allocate a committed page
									  PAGE_READWRITE);						  // Read/write access

		if(lpvResult != NULL) {
			auto end =
			  (pages.second > m_PageState.size()) ? std::end(m_PageState) : std::begin(m_PageState) + pages.second;
			std::fill(std::begin(m_PageState) + start, end, state::COMMITED);
		} else {
			return false;
		}
	}
#endif
	return true;
}

bool region::deallocate(segment& segment) {
	return m_Allocator->deallocate(segment);
}

bool region::deallocate(std::optional<segment>& segment) {
	return ((segment) ? m_Allocator->deallocate(segment.value()) : false);
}

void region::compact() {
	m_Allocator->compact();
}

void region::decommit_unused() {
	if(!m_Allocator->is_physically_backed()) {
		return;
	}
	// we only keep track on windows what the pages are
#if defined(USE_WIN32)
	auto committed {m_Allocator->available()};
	for(auto commit : committed) {
		auto pages = page_range(commit);

		auto loc = (std::uintptr_t)(m_Base) + (m_PageSize * pages.first);

		VirtualFree((void*)loc,									  // Next page to commit
					m_PageSize * (pages.second - pages.first),	  // Page size, in bytes
					MEM_DECOMMIT);								  // Read/write access

		auto end = (pages.second > m_PageState.size()) ? std::end(m_PageState) : std::begin(m_PageState) + pages.second;
		std::fill(std::begin(m_PageState) + pages.first, end, state::RESERVED);
	}
#endif
}

#if defined(USE_WIN32)
std::pair<size_t, size_t> region::page_range(const memory::range_t& range) {
	auto offset		 = range.begin - (std::uintptr_t)(m_Base);
	auto start_index = (offset - (offset % m_PageSize)) / m_PageSize;
	offset			 = range.end - (std::uintptr_t)(m_Base);
	auto end_index	 = (offset + (m_PageSize - (offset % m_PageSize))) / m_PageSize;
	if(end_index > m_PageState.size())
		end_index = m_PageState.size();
	return {start_index, end_index};
}
#endif

#ifdef USE_WIN32
	#undef USE_WIN32
#endif
#ifdef USE_POSIX
	#undef USE_POSIX
#endif
#ifdef USE_GENERIC
	#undef USE_GENERIC
#endif
