#pragma once

namespace psl
{
	struct UID;
	namespace meta
	{
		class file;
	}
} // namespace psl

namespace core::resource
{
	class cache;

	template <typename T>
	class handle;

	template <typename T>
	class indirect_handle;
} // namespace core::resource