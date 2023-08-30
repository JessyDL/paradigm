#pragma once
#include "allocator.hpp"
#include "psl/expected.hpp"
#include "psl/platform_def.hpp"
#include "range.hpp"
#include "segment.hpp"
#include <vector>

namespace memory {
/// \brief defines a region of memory that *might* be physically backed depending on the allocator.
///
/// memory regions allow for easy managing of virtual memory in a platform abstract way.
/// it provides some helper utilities to allocate correctly sized segments for the given type
/// and even allows constructor/destructors to be called (using the
/// memory::region::create()/memory::region::destroy() methods). Memory regions do not allocate memory in one go,
/// they reserve and "grow to" the specified size (like how virtual memory behaves). How they do this is platform
/// specific however, but should not be of concern for the end-user.
class region {
	friend class allocator_base;

	/// \brief describes the various states memory can be in.
	enum class state {
		RESERVED = 0,
		COMMITED = 1,
		DEFERRED = 2,	 // someone else manages this, likely a sub-region
		RELEASED = -1
	};

	/// \brief specialized constructor used for internal usage
	/// \details we keep this one private as it relies on certain conditions on the size of the
	/// segment (being aligned to pages, and starting at a page).
	region(region& parent,
		   memory::segment& segment,
		   size_t pageSize,
		   size_t alignment,
		   allocator_base* allocator = new default_allocator());

  public:
	/// \brief constructs a region of atleast the given \a size, using the \a alignment value.
	/// \param[in] size the minimum size you wish to create a virtual region for.
	/// \param[in] alignment the alignment value of the region.
	/// \param[in] allocator the allocator that should be used internally.
	/// \warning \a allocators should not be shared unless the allocator itself supports such a behaviour.
	region(size_t size, size_t alignment, allocator_base* allocator = new default_allocator());

	~region();
	region(const region& other) = delete;
	region(region&& other);
	region& operator=(const region& other) = delete;
	region& operator=(region&& other);

	/// \brief tries to allocate a segment of atleast the given \a size.
	/// \returns a memory::segment on success.
	/// \param[in] size the minimum size of the segment (will be aligned to the alignment rules).
	std::optional<segment> allocate(size_t size);

	/// \brief creates a sub-region of atleast that size.
	///
	/// Creates a sub region of atleast the given size. The region could be bigger depending on
	/// certain factors such as alignment, etc..
	/// Destruction/cleanup happens automatically through the destructor of the created region.
	[[nodiscard]] std::optional<memory::region>
	create_region(size_t size, std::optional<size_t> alignment, allocator_base* allocator = new default_allocator());


	template <typename T>
	std::optional<segment> allocate() {
		return allocate(sizeof(T));
	}

	bool owns(const memory::segment& segment) const noexcept { return m_Allocator->owns(segment); }


	// specialized version of allocate that will create the given type instead
	// when using this version, please also call destroy on the result when you're done
	// this method will fail when no memory is remaining in the region
	template <typename T, typename... Args>
	std::optional<std::reference_wrapper<T>> create(Args&&... args) {
		auto seg {allocate(sizeof(T))};
		if(seg) {
#ifdef DBG_NEW
	#undef new
#endif
			T* item = new((char*)seg.value().range().begin) T(std::forward<Args>(args)...);
#ifdef DBG_NEW
	#define new DBG_NEW
#endif
			return *item;
		}
		return {};
	}

	template <typename T>
	bool destroy(T& target) {
		memory::range_t temp_range {(std::uintptr_t)((void*)(&target)), (std::uintptr_t)((void*)(&target)) + sizeof(T)};
		memory::segment seg(temp_range, m_Allocator->is_physically_backed());
		if(deallocate(seg)) {
			target.~T();
			return true;
		}
		return false;
	}
	template <typename T>
	bool destroy(std::optional<T>& target) {
		return ((target) ? destroy(target.value()) : false);
	}

	bool deallocate(segment& segment);
	bool deallocate(std::optional<segment>& segment);
	allocator_base* allocator() const {
		return m_Allocator;
	};
	void compact();
	void decommit_unused();
	void* data() const {
		return m_Base;
	}
	size_t size() const {
		return m_Size;
	}
	size_t alignment() const {
		return m_Alignment;
	}
	memory::range_t range() const {
		return memory::range_t {(std::uintptr_t)(m_Base), (std::uintptr_t)(m_Base) + m_Size};
	}

  private:
	bool erase_region(memory::region& child);


	void* m_Base {nullptr};
	memory::region* m_Parent {nullptr};
	std::vector<memory::region*> m_Children;
	size_t m_Size;
	size_t m_Alignment;
	// this will return true if the region can be allocated onto (i.e. it is backed by real memory)
	bool commit(const memory::range_t& range);
	allocator_base* m_Allocator {nullptr};

#ifdef PLATFORM_WINDOWS
	std::pair<size_t, size_t> page_range(const memory::range_t& range);
	std::vector<state> m_PageState;
#endif
	size_t m_PageSize {0u};
};
}	 // namespace memory
