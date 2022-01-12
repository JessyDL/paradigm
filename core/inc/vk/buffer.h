#pragma once
#include "gfx/types.h"
#include "psl/memory/segment.h"
#include "resource/resource.hpp"
#include "vk/ivk.h"
#include <optional>


namespace core::data
{
	class buffer_t;
}

namespace core::ivk
{
	class context;
}

namespace core::ivk
{
	/// \brief maps a memory region and interfaces with the driver for read/writes
	///
	/// buffers are used to map regions of memory that the driver should know about, either
	/// because they will be used to map GPU resources (geometry, textures, etc..),
	/// or because they are used for synchronising information between CPU/GPU (compute results).
	/// This class will handle most of the needs for synchonising, and how-to upload the data to the
	/// relevant locations as well as managing the internals.
	class buffer_t
	{
	  public:
		/// \brief constructs a buffer from the given buffer_data, as well as optionally sets a staging resource.
		/// \param[in] buffer_data the data source to bind to this buffer. (see note for more info)
		/// \param[in] staging_buffer the staging buffer to use in case staging is needed. (see warning for more info)
		/// \warning it is recommended to give a staging_buffer unless you know this is a host-only resource.
		/// failing to give a staging resource to a device-local region that cannot be accessed by the host
		/// will result in performance degradation as it has to keep making temporary staging buffers instead.
		/// \note buffer_data dictates the size, and alignment of this buffer resource. In the event that the
		/// allignment is incorrect, a suitable warning (and potential override) will be supplied.
		/// If the supplied buffer_data is non-virtual (i.e. backed by real memory location), then the resource
		/// will be duplicated and accessible for read access through the core::data::buffer_t handle directly.
		buffer_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::ivk::context> context,
			   core::resource::handle<core::data::buffer_t> buffer_data,
			   std::optional<core::resource::handle<core::ivk::buffer_t>> staging_buffer = std::nullopt);
		~buffer_t();
		buffer_t(const buffer_t&) = delete;
		buffer_t(buffer_t&&)	  = delete;
		buffer_t& operator=(const buffer_t&) = delete;
		buffer_t& operator=(buffer_t&&) = delete;

		// when optimize is set to true it can allocate multiple ranges in one segment if possible.
		// it will return ranges with local begin/end values relative to the segment.
		// note that for the memory::region, this is considered one allocation instead of several.
		/// \brief tries to reserve a region of memory of *at least* the given size in thye buffer.
		///
		/// \param[in] size the minimum size to allocate
		/// \returns a memory::segment on success.
		[[nodiscard]] std::optional<memory::segment> reserve(vk::DeviceSize size);


		size_t free_size() const noexcept;
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
		[[nodiscard]] std::vector<std::pair<memory::segment, memory::range_t>> reserve(std::vector<vk::DeviceSize> sizes,
																					 bool optimize = false);


		/// \brief tries to find the appropriate method to update the buffer with the commit instructions.
		///
		/// this method tries to commit the given instruction into the buffer. depending on the type of buffer
		/// how this does that can differ greatly.
		/// \warning if the buffer is device local and no staging buffer is known, it will try to create a staging
		/// buffer to facilitate transfers this has a performance overhead. \param[in] instructions all the instructions
		/// you wish to send to the GPU in this batch. \returns success if the instruction has been sent. \note this
		/// method will try to figure out the best way to send this set of instructions to the GPU, possibly merging
		/// instructions together.
		bool commit(std::vector<core::gfx::commit_instruction> instructions);

		/// \brief marks the specific region of memory available again.
		///
		/// \param[in] segment the region you wish to free up.
		/// \returns success in case the region was freed. note that in case the buffer was not the owner of the
		/// memory::segment, false will be returned. \note no actual memory will be freed. driver resources are created
		/// at the start (constructor) and can only be freed completely (destructor). there is no intermediate unless
		/// you copy over the resources to a new, smaller buffer. Check copy_from() for that.
		bool deallocate(memory::segment& segment);
		// bool map(const memory::region& region, const memory::segment& segment);
		// bool map(const memory::region& region, const memory::segment& segment, const memory::range_t& sub);

		// bool copy_from(const buffer& other, std::optional<vk::DeviceSize> size = {}, std::optional<vk::DeviceSize>
		// dstOffset = {}, std::optional<vk::DeviceSize> srcOffset = {});
		/// \brief allows you to copy from one buffer into another.
		/// \param[in] other the buffer to copy from into this instance.
		/// \param[in] copyRegions the batch of copy instructions.
		/// \returns true in case the instructions were successfully uploaded to the GPU.
		bool copy_from(const buffer_t& other, const std::vector<vk::BufferCopy>& copyRegions);

		// bool set(const void* data, vk::DeviceSize size, std::optional<vk::DeviceSize> dstOffset = {},
		// std::optional<vk::DeviceSize> srcOffset = {});
		bool set(const void* data, std::vector<vk::BufferCopy> commands);

		/// \returns in case the GPU/driver is busy.
		/// \note could be true for various situations, in case it is busy uploading, or hasn't finalized everything yet
		/// amongst other.
		bool is_busy() const;

		/// \brief forcibly wait until all operations are done, or the timeout has been reached.
		void wait_until_ready(uint64_t timeout = UINT64_MAX) const;

		/// \brief makes the buffer available on the host.
		/// \param[in] compressed_copy when compressed is true, then the buffer will collapse all empty regions and
		/// return a buffer that is the size of all actual committed memory. \returns a handle to a HOST_VISIBLE buffer
		/// on success.
		std::optional<resource::handle<buffer_t>> copy_to_host(bool compressed_copy = true) const;

		// this variation creates a new buffer that consists out of the regions you wanted to copy from.

		/// \brief makes the buffer available on the host.
		/// \param[in] copyRegions the regions the new buffer will consist out of. The new buffer will be the size of
		/// the accumulate size of the copyRegions (+ alignment rules). \returns a handle to a HOST_VISIBLE buffer on
		/// success.
		std::optional<resource::handle<buffer_t>> copy_to_host(const std::vector<vk::BufferCopy>& copyRegions) const;

		/// \returns the vk::Buffer handle.
		const vk::Buffer& gpu_buffer() const;

		/// \returns the internal buffer data.
		core::resource::handle<core::data::buffer_t> data() const;

		/// \returns the vulkan descriptor buffer info.
		vk::DescriptorBufferInfo& buffer_info();

	  private:
		bool map(const void* data, vk::DeviceSize size, vk::DeviceSize offset);
		core::resource::handle<core::ivk::context> m_Context;
		vk::DescriptorBufferInfo m_Descriptor;

		vk::Buffer m_Buffer;
		vk::Buffer m_DoubleBuffer;
		vk::DeviceMemory m_Memory;
		vk::Fence m_BufferCompleted;
		vk::CommandBuffer m_CommandBuffer;

		core::resource::handle<core::data::buffer_t> m_BufferDataHandle;
		core::resource::handle<core::ivk::buffer_t> m_StagingBuffer;
		core::resource::cache_t& m_Cache;

		psl::UID m_UID;
	};

}	 // namespace core::ivk
