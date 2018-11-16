#include "stdafx_psl.h"
#include "file.h"
#include "platform_def.h"
#include "string_utils.h"
#include "Logger.h"

using namespace utility::os;



file::file(psl::string_view filename, mode mode, method method, std::optional<size_t> offset, std::optional<size_t> length)
{
#ifdef PLATFORM_WINDOWS
	auto file_flags = (mode == mode::READ)?GENERIC_READ: (mode== mode::WRITE)? GENERIC_WRITE: GENERIC_READ | GENERIC_WRITE;
	auto method_flags = 0L;
	switch (method)
	{
	case method::OPEN:
		method_flags = OPEN_EXISTING;
		break;
	case method::OPEN_OR_CREATE:
		method_flags = OPEN_ALWAYS;
		break;
	case method::CREATE:
		method_flags = CREATE_NEW;
		break;
	case method::TRUNCATE:
		method_flags = TRUNCATE_EXISTING;
		break;
	case method::TRUNCATE_OR_CREATE:
		method_flags = CREATE_ALWAYS;
		break;
	}
	m_File = CreateFile(psl::to_platform_string(filename).data(), file_flags, 0, NULL, method_flags, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_File == INVALID_HANDLE_VALUE)
	{
		m_File = {};
		return;
	}

	DWORD dwSysGran;      // system allocation granularity
	SYSTEM_INFO SysInfo;  // system information; used to get granularity

	
	if(LARGE_INTEGER file_size; ::GetFileSizeEx(m_File.value(), &file_size))
	{
		m_Size = static_cast<size_t>(file_size.QuadPart);
	}
	else
	{
		close();
		return;
	}

	// Get the system allocation granularity.
	GetSystemInfo(&SysInfo);
	dwSysGran = SysInfo.dwAllocationGranularity;

	size_t offset_val = offset.value_or(0u);
	m_Size = length.value_or(m_Size);
	auto file_start = (offset_val / dwSysGran) * dwSysGran;
	auto view_size = (offset_val % dwSysGran) + m_Size;
	auto map_size = offset_val + m_Size;
	auto view_delta = offset_val - file_start;

	auto permission = (mode == mode::READ) ? PAGE_READONLY : PAGE_READWRITE;
	m_Map = CreateFileMapping(m_File.value(),          // current file handle
		NULL,           // default security
		permission, // read/write permission
		map_size >> 32,              // size of mapping object, high
		map_size & 0xffffffff,  // size of mapping object, low
		NULL);          // name of mapping object

	if(m_Map.value() == NULL)
	{
		m_Map = {};
		close();
		return;
	}

	auto file_access = (mode == mode::READ) ? FILE_MAP_READ : (mode == mode::WRITE) ? FILE_MAP_WRITE : FILE_MAP_ALL_ACCESS;
	m_MapView = MapViewOfFile(m_Map.value(), file_access, file_start >> 32, file_start & 0xffffffff, view_size);

	if(m_MapView.value() == NULL)
	{
		m_MapView = {};
		close();
		return;
	}
	// Calculate the pointer to the data.
	m_Data = (psl::char_t *)m_MapView.value() + view_delta;

	if(length)
	{
		m_ContentSize = length.value();
	}
	else
	{
		auto view_ = psl::string_view{&m_Data[0], m_Size};

		size_t index = m_Size;
		for(auto it = std::end(view_) - 1; it != std::begin(view_); --it)
		{
			if(*it != _T('\0'))
			{
				break;
			}
			--index;
		}
		m_ContentSize = index;
	}
#endif
}



file::~file()
{
	close();
}

bool file::close()
{
#ifdef PLATFORM_WINDOWS
	bool success = true;
	if (m_MapView && !UnmapViewOfFile(m_MapView.value()))
	{
		LOG_ERROR("Error occurred during the closing of the mmap view with error: ", GetLastError());
		success = false;
	}
	else
	{
		m_MapView = {};
	}
	if (m_Map && !CloseHandle(m_Map.value()))
	{
		LOG_ERROR("Error occurred during the closing of the mmap handle with error: ", GetLastError());
		success = false;
	}
	else
	{
		m_Map = {};
	}
	if (m_File && !CloseHandle(m_File.value()))
	{
		LOG_ERROR("Error occurred during the closing of the file handle with error: ", GetLastError());
		success = false;
	}
	else
	{
		m_File = {};
	}
	return success;
#else
	throw std::runtime_error("not implemented");
#endif
}
