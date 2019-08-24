#pragma once
#include "psl/meta.h"

namespace core::resource
{
	template <typename T>
	class tag
	{
	  public:
		tag(const psl::UID& uid) : m_UID(uid){};
		~tag()			= default;
		tag(const tag&) = default;
		tag(tag&&)		= default;
		tag& operator=(const tag&) = default;
		tag& operator=(tag&&) = default;

		operator const psl::UID&() const noexcept { return m_UID; };
		const psl::UID& uid() const noexcept { return m_UID; };

	  private:
		psl::UID m_UID{};
	};
}