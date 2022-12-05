#pragma once
#include "psl/ustring.hpp"
#include <optional>

namespace utility::os
{
	class file
	{
	  public:
		enum mode
		{
			READ,
			WRITE,
			READ_AND_WRITE
		};

		enum method
		{
			// creates if it doesn't exist, if it does, it fails
			CREATE,

			// truncates or creates if it doesn't exist
			TRUNCATE_OR_CREATE,

			// opens, or fails if it doesn't exist
			OPEN,

			// opens, or creates if it doesn't exist
			OPEN_OR_CREATE,

			// trunctates or fails if it doesn't exist
			TRUNCATE
		};


		file(psl::string_view filename,
			 mode mode					  = mode::READ_AND_WRITE,
			 method method				  = method::OPEN_OR_CREATE,
			 std::optional<size_t> offset = {},
			 std::optional<size_t> length = {});
		~file();


		operator bool() const { return m_Data; }

		std::optional<psl::string_view> view() const
		{
			if(!m_Data) return {};
			return psl::string_view {&m_Data[0], m_ContentSize};
		}

		std::optional<psl::char_t*> data() const
		{
			if(!m_Data) return {};
			return m_Data;
		}

	  private:
		bool close();

		psl::char_t* m_Data {nullptr};
		size_t m_Size {0};
		size_t m_ContentSize {0};
#ifdef PLATFORM_WINDOWS
		std::optional<void*> m_File;
		std::optional<void*> m_Map;
		std::optional<void*> m_MapView;
#endif
	};
}	 // namespace utility::os
