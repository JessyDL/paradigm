#pragma once

namespace psl
{
	struct UID;
	namespace meta
	{
		class file;
	}
}	 // namespace psl

namespace core::resource
{
	/// \brief represents the various states a resource can be in.
	enum class status
	{
		initial	  = 0,
		loading	  = 1,
		loaded	  = 2,
		unloading = 3,
		unloaded  = 4,
		missing	  = -1,
		/// \brief special sentinel state in case something disastrous has happened, like OOM
		invalid = -2
	};


	class cache_t;

	template <typename T>
	class handle;

	template <typename T>
	class weak_handle;

	template <typename... Ts>
	struct alias;

	template <typename T>
	class tag;

	struct metadata;


	template <typename T>
	struct resource_traits
	{
		using meta_type = psl::meta::file;
	};
}	 // namespace core::resource

namespace core::resource2
{
	/// \brief represents the various states a resource can be in.
	enum class state
	{
		INITIAL	  = 0,
		LOADING	  = 1,
		LOADED	  = 2,
		UNLOADING = 3,
		UNLOADED  = 4,
		MISSING	  = -1,
		INVALID	  = -2,
	};

	class cache_t;

	template <typename T>
	class handle;

	// template <typename T>
	// class weak_handle;

	// template <typename... Ts>
	// struct alias;

	template <typename T>
	class tag;

	struct metadata;


	template <typename T>
	struct resource_traits
	{
		using meta_type = psl::meta::file;
	};
}	 // namespace core::resource2
