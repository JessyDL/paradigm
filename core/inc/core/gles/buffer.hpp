#pragma once
#include "core/gfx/types.hpp"
#include "core/gles/types.hpp"
#include "core/resource/resource.hpp"
#include "psl/array.hpp"
#include "psl/memory/segment.hpp"
#include <optional>

namespace core::data {
class buffer_t;
}

namespace core::igles {
class buffer_t {
  public:
	buffer_t(core::resource::cache_t& cache,
			 const core::resource::metadata& metaData,
			 psl::meta::file* metaFile,
			 core::resource::handle<core::data::buffer_t> buffer_data);

	~buffer_t();
	buffer_t(const buffer_t&)			= delete;
	buffer_t(buffer_t&&)				= delete;
	buffer_t& operator=(const buffer_t) = delete;
	buffer_t& operator=(buffer_t&&)		= delete;

	bool set(const void* data, std::vector<core::gfx::memory_copy> commands);
	bool set(std::vector<core::gfx::memory_copy> commands);

	inline GLuint id() const noexcept { return m_Buffer; };

	// when optimize is set to true it can allocate multiple ranges in one segment if possible.
	// it will return ranges with local begin/end values relative to the segment.
	// note that for the memory::region, this is considered one allocation instead of several.
	/// \brief tries to reserve a region of memory of *at least* the given size in the buffer.
	///
	/// \param[in] size the minimum size to allocate
	/// \returns a memory::segment on success.
	[[nodiscard]] std::optional<memory::segment> allocate(size_t size);

	/// \brief tries to reserve all requested sizes in the memory::region of this buffer.
	///
	/// optmized version of reserve() that works on a batch of data. It will try to satisfy allocating
	/// regions of memory of atleast the given accumulative size. Depending on the optimize boolean parameter's
	/// value this can be in one optimized memory::segment (true), or in equal amount of memory::segments as there
	/// were elements in the sizes requested container (false).
	/// when optimize is true, the returned memory::range_t in the pair signifies the offset from the start of the
	/// segment (where the actual memory you requested resides). when optimize is false, range always starts at 0,
	/// and ends at the actual allocated size. \returns a vector with as many contained elements as the requested
	/// sizes container has. \param[in] sizes a container with minimum sizes you want to request. \param[in]
	/// optimize signifies if we should we try to collapse multiple memory::segments into one segment if possible,
	/// avoiding fragmentation and overhead.
	[[nodiscard]] std::vector<std::pair<memory::segment, memory::range_t>> allocate(psl::array<size_t> sizes,
																					bool optimize = false);

	/// \brief marks the specific region of memory available again.
	///
	/// \param[in] segment the region you wish to free up.
	/// \returns success in case the region was freed. note that in case the buffer was not the owner of the
	/// memory::segment, false will be returned. \note no actual memory will be freed. driver resources are created
	/// at the start (constructor) and can only be freed completely (destructor). there is no intermediate unless
	/// you copy over the resources to a new, smaller buffer. Check copy_from() for that.
	bool deallocate(memory::segment& segment);

	const core::data::buffer_t& data() const noexcept { return m_BufferDataHandle.value(); }

	bool copy_from(const buffer_t& other, const psl::array<core::gfx::memory_copy>& ranges);

	bool commit(const psl::array<core::gfx::commit_instruction>& instructions);
	size_t free_size() const noexcept;

  private:
	GLuint m_Buffer;
	GLint m_BufferType;
	core::resource::handle<core::data::buffer_t> m_BufferDataHandle;
	psl::UID m_UID;
};
}	 // namespace core::igles
