#include "memory/raw_region.h"
#include <algorithm>
#include "platform_def.h"
#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#endif
#if defined(PLATFORM_LINUX) || defined (PLATFORM_ANDROID)
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "assertions.h"
#include "logging.h"
using namespace memory;


raw_region::raw_region(uint64_t size)
{
#if defined(PLATFORM_WINDOWS)
		SYSTEM_INFO sSysInfo;         // Useful information about the system
		GetSystemInfo(&sSysInfo);     // Initialize the structure.
		m_PageSize = sSysInfo.dwPageSize;

		uint64_t pages = (size + (size % m_PageSize)) / m_PageSize;
		pages = (pages == 0u) ? 1u : pages;
		m_Size = pages * m_PageSize;
		m_Base = (void*)VirtualAlloc(
			NULL,                 // System selects address
			m_Size,					// Size of allocation
			MEM_RESERVE | MEM_COMMIT,          // Allocate reserved pages
			PAGE_READWRITE);       // Protection = no access


		if(m_Base == nullptr)
		{
			// todo error state
			__debugbreak();
		}
#else
		m_PageSize = sysconf(_SC_PAGE_SIZE);

		uint64_t pages = (size + (size % m_PageSize)) / m_PageSize;
		pages = (pages == 0u) ? 1u : pages;

		m_Size = m_PageSize * pages;

		auto addr = mmap(NULL, m_Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if(addr == MAP_FAILED)
		{
			exit(EXIT_FAILURE);
		}
		m_Base = (std::byte *)addr;

#endif
}

raw_region::~raw_region()
{
#ifdef PLATFORM_WINDOWS
	VirtualFree(
		m_Base,       // Base address of block
		0,             // Bytes of committed pages
		MEM_RELEASE);  // Decommit the pages
#else
	if (munmap(m_Base, sizeof(int)) == -1)
	{
		LOG_ERROR("munmap()() failed");
		exit(EXIT_FAILURE);
	}
#endif
}

raw_region::raw_region(const raw_region& other):
	m_Base(other.m_Base),
	m_Size(other.m_Size),
	m_PageSize(other.m_PageSize)
{
}

raw_region& raw_region::operator=(const raw_region& other)
{
	if (this != &other)
	{
		m_Base = other.m_Base;
		m_Size = other.m_Size;
		m_PageSize = other.m_PageSize;

	}
	return *this;
}

raw_region::raw_region(raw_region&& other):
	m_Base(other.m_Base),
	m_Size(other.m_Size),
	m_PageSize(other.m_PageSize)
{
	other.m_Base = nullptr;
}

raw_region& raw_region::operator=(raw_region&& other)
{
	if (this != &other)
	{
		m_Base = other.m_Base;
		m_Size = other.m_Size;
		m_PageSize = other.m_PageSize;

		other.m_Base = nullptr;
	}
	return *this;
}