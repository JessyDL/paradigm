#include "psl/memory/raw_region.hpp"
#include "psl/platform_def.hpp"
#include <algorithm>
#if defined(PLATFORM_WINDOWS)
	#include <Windows.h>
#endif
#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID) || defined(PLATFORM_MACOS)
	#include <sys/mman.h>
	#include <unistd.h>
#endif
#include "psl/assertions.hpp"
#include "psl/logging.hpp"
#include <cstddef>	  // std::byte
using namespace memory;


raw_region::raw_region(std::uint64_t size) {
	if(size == 0) {
		m_Base	   = nullptr;
		m_Size	   = 0;
		m_PageSize = 0;
		return;
	}
#if defined(PLATFORM_GENERIC)
	m_PageSize = size;
	m_Base	   = malloc(size);
	m_Size	   = size;
	psl_assert(m_Base != nullptr, "failed to allocate {} bytes", size);
#elif defined(PLATFORM_WINDOWS)
	SYSTEM_INFO sSysInfo;		 // Useful information about the system
	GetSystemInfo(&sSysInfo);	 // Initialize the structure.
	m_PageSize = sSysInfo.dwPageSize;

	m_Size = (size + m_PageSize - 1) / m_PageSize * m_PageSize;

	m_Base = (void*)VirtualAlloc(NULL,						  // System selects address
								 m_Size,					  // Size of allocation
								 MEM_RESERVE | MEM_COMMIT,	  // Allocate reserved pages
								 PAGE_READWRITE);			  // Protection = no access


	if(m_Base == nullptr) {
		// todo error state
		__debugbreak();
	}
#else
	m_PageSize = sysconf(_SC_PAGE_SIZE);

	m_Size = (size + m_PageSize - 1) / m_PageSize * m_PageSize;

	auto addr = mmap(NULL, m_Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(addr == MAP_FAILED) {
		exit(EXIT_FAILURE);
	}
	m_Base = (std::byte*)addr;

#endif
}

raw_region::~raw_region() {
	release();
}

raw_region::raw_region(const raw_region& other)
	: m_Base(other.m_Base), m_Size(other.m_Size), m_PageSize(other.m_PageSize) {}

raw_region& raw_region::operator=(const raw_region& other) {
	if(this != &other) {
		m_Base	   = other.m_Base;
		m_Size	   = other.m_Size;
		m_PageSize = other.m_PageSize;
	}
	return *this;
}

raw_region::raw_region(raw_region&& other) : m_Base(other.m_Base), m_Size(other.m_Size), m_PageSize(other.m_PageSize) {
	other.m_Base	 = nullptr;
	other.m_Size	 = 0;
	other.m_PageSize = 0;
}

raw_region& raw_region::operator=(raw_region&& other) {
	if(this != &other) {
		std::swap(m_Base, other.m_Base);
		std::swap(m_Size, other.m_Size);
		std::swap(m_PageSize, other.m_PageSize);
		other.release();
	}
	return *this;
}

void raw_region::release() noexcept {
	if(!m_Base) {
		return;
	}
#if defined(PLATFORM_GENERIC)
	free(m_Base);
#elif defined(PLATFORM_WINDOWS)
	VirtualFree(m_Base,			 // Base address of block
				0,				 // Bytes of committed pages
				MEM_RELEASE);	 // Decommit the pages
#else
	if(munmap(m_Base, sizeof(int)) == -1) {
		LOG_ERROR("munmap()() failed");
		exit(EXIT_FAILURE);
	}
#endif
	m_Size	   = 0;
	m_PageSize = 0;
	m_Base	   = nullptr;
}
