#pragma once

namespace psl
{
	struct UID;
	namespace meta
	{
		class file;
	}
}

namespace core::resource
{
	// creates a deep_copy of the handle that will be detached from the original handle
	struct deep_copy_t
	{
		explicit deep_copy_t() = default;
	};
	constexpr deep_copy_t deep_copy{};


	/// \brief represents the various states a resource can be in.
	enum class state
	{
		NOT_LOADED = 0,
		LOADING	= 1,
		LOADED	 = 2,
		UNLOADING  = 3,
		UNLOADED   = 4,
		MISSING	= -1,
		/// \brief special sentinel state in case something disastrous has happened, like OOM
		INVALID = -2
	};


	class cache;

	template <typename T>
	class handle;

	template <typename T>
	class indirect_handle;

	template <typename T>
	class tag;

	template <typename T>
	static handle<T> create_shared(cache& cache, const psl::UID& uid);
} // namespace core::resource