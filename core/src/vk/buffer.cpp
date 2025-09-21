#include "core/vk/buffer.hpp"
#include "core/data/buffer.hpp"
#include "core/logging.hpp"
#include "core/vk/context.hpp"
#include "core/vk/conversion.hpp"

#include "tracy/Tracy.hpp"

using namespace psl;
using namespace core;
using namespace core::gfx;
using namespace core::ivk;
using namespace core::resource;


// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkCmdUpdateBuffer.html
static const size_t max_size_set {65535};

static struct CopyFromManager {
	struct CopyJob {
		const buffer_t* source;
		const buffer_t* target;
		std::vector<vk::BufferCopy> copyRegions;
		std::function<void()> on_finish;
		CopyJob(const buffer_t* source,
				const buffer_t* target,
				const std::vector<vk::BufferCopy>& copyRegions,
				std::function<void()> on_finish)
			: source(source), target(target), copyRegions(copyRegions), on_finish(on_finish) {}
	};
	void schedule(const buffer_t& other,
				  const buffer_t& source,
				  const std::vector<vk::BufferCopy>& copyRegions,
				  std::function<void()> on_finish) {
		auto lock = std::scoped_lock(m_Mutex1);
		m_Data.emplace_back(&other, &source, copyRegions, on_finish);
	}

	void execute(vk::Queue queue, vk::CommandBuffer commandBuffer, vk::Fence fence, vk::Device device) {
		while(true) {
			m_Mutex1.lock();
			std::unique_lock execLock(m_Mutex2, std::try_to_lock);
			if(!execLock.owns_lock()) {
				m_Mutex1.unlock();
				return;
			}

			std::vector<CopyJob> data {};
			{
				data = std::move(m_Data);
				m_Data.clear();
				m_Mutex1.unlock();
			}
			if(data.empty()) {
				return;
			}
			if(device.getFenceStatus(fence) != vk::Result::eSuccess) {
				if(auto res = device.waitForFences(1, &fence, VK_TRUE, UINT64_MAX);
				   !core::utility::vulkan::check(res)) {
					core::ivk::log->critical(
					  "could not wait for the fence to become signaled in an ivk::buffer_t copy operation. Reason: "
					  "{}",
					  vk::to_string(res));
					std::abort();
				}
			}

			vk::CommandBufferBeginInfo cmdBufferBeginInfo;
			cmdBufferBeginInfo.pNext = NULL;

			std::vector<vk::BufferMemoryBarrier> barriers {};
			std::vector<vk::MemoryBarrier> membarriers {};
			barriers.resize(data.size());
			membarriers.resize(data.size());
			auto barrier	= barriers.begin();
			auto membarrier = membarriers.begin();

			// Put buffer region copies into command buffer
			// Note that the staging buffer must not be deleted before the copies
			// have been submitted and executed
			core::utility::vulkan::check(commandBuffer.begin(&cmdBufferBeginInfo));
			for(auto& [other, source, copyRegions, on_finish] : data) {
				barrier->pNext				 = NULL;
				barrier->srcAccessMask		 = vk::AccessFlagBits::eTransferWrite;
				barrier->dstAccessMask		 = vk::AccessFlagBits::eVertexAttributeRead;
				barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier->buffer				 = source->gpu_buffer();
				barrier->offset				 = 0;
				barrier->size				 = VK_WHOLE_SIZE;

				membarrier->pNext		  = NULL;
				membarrier->srcAccessMask = vk::AccessFlagBits::eTransferWrite;
				membarrier->dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead;
				commandBuffer.copyBuffer(
				  other->gpu_buffer(), source->gpu_buffer(), (uint32_t)copyRegions.size(), copyRegions.data());
				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
											  vk::PipelineStageFlagBits::eVertexInput |
												vk::PipelineStageFlagBits::eVertexShader,
											  vk::DependencyFlagBits(),
											  1,
											  &*membarrier,
											  0,
											  nullptr,
											  0,
											  nullptr);
				++barrier;
			}
			if(auto res = commandBuffer.end(); !core::utility::vulkan::check(res)) {
				core::ivk::log->critical(
				  "could not end the command buffer for an ivk::buffer_t copy operation. Reason: {}",
				  vk::to_string(res));
				std::abort();
			}
			// Submit copies to the queue
			vk::SubmitInfo copySubmitInfo;
			copySubmitInfo.commandBufferCount = 1;
			copySubmitInfo.pCommandBuffers	  = &commandBuffer;
			device.resetFences(fence);
			core::utility::vulkan::check(queue.submit(1, &copySubmitInfo, fence));
			if(auto res = queue.waitIdle(); !core::utility::vulkan::check(res)) {
				core::ivk::log->critical(
				  "could not wait for the queue to become idle after an ivk::buffer_t copy operation. "
				  "Reason: {}",
				  vk::to_string(res));
				std::abort();
			}
			for(auto& [other, source, copyRegions, on_finish] : data) {
				if(on_finish) {
					on_finish();
				}
			}
		}
	}

	std::mutex m_Mutex1 {};
	std::mutex m_Mutex2 {};

	std::vector<CopyJob> m_Data {};
} copy_from_manager {};

buffer_t::buffer_t(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   handle<context> context,
				   handle<data::buffer_t> buffer_data,
				   std::optional<core::resource::handle<core::ivk::buffer_t>> staging_buffer)
	: m_Context(context), m_BufferDataHandle(std::move(buffer_data)), m_Cache(cache), m_UID(metaData.uid),
	  m_StagingBuffer(staging_buffer.value_or(core::resource::handle<core::ivk::buffer_t> {})) {
	ZoneScoped;
	core::ivk::log->info("creating an ivk::buffer_t of {0} bytes size.", m_BufferDataHandle->size());
	vk::MemoryRequirements memReqs;
	auto& region   = m_BufferDataHandle->region();
	auto alignment = region.alignment();
	auto type	   = conversion::to_vk(m_BufferDataHandle->usage());

	if(type & vk::BufferUsageFlagBits::eUniformBuffer &&
	   alignment != m_Context->properties().limits.minUniformBufferOffsetAlignment) {
		core::ivk::log->warn(
		  "trying to create an ivk::buffer_t [UID: {0} ] with incorrect alignment, alignment is: {1}, but should be: "
		  "{2}",
		  m_UID.to_string(),
		  alignment,
		  m_Context->properties().limits.minUniformBufferOffsetAlignment);
	} else if(type & vk::BufferUsageFlagBits::eStorageBuffer &&
			  alignment != m_Context->properties().limits.minStorageBufferOffsetAlignment) {
		core::ivk::log->warn(
		  "trying to create an ivk::buffer_t [UID: {0} ] with incorrect alignment, alignment is: {1}, but should be: "
		  "{2}",
		  m_UID.to_string(),
		  alignment,
		  m_Context->properties().limits.minStorageBufferOffsetAlignment);
	}

	vk::MemoryAllocateInfo memAllocInfo;
	memAllocInfo.pNext			 = NULL;
	memAllocInfo.allocationSize	 = 0;
	memAllocInfo.memoryTypeIndex = 0;

	// Vertex buffer
	vk::BufferCreateInfo bufCreateInfo;
	bufCreateInfo.pNext = NULL;
	bufCreateInfo.usage = type;
	bufCreateInfo.size	= m_BufferDataHandle->size();
	bufCreateInfo.flags = vk::BufferCreateFlagBits();

	core::utility::vulkan::check(m_Context->device().createBuffer(&bufCreateInfo, nullptr, &m_Buffer));

	memReqs = m_Context->device().getBufferMemoryRequirements(m_Buffer), memAllocInfo.allocationSize = memReqs.size;

	m_Context->memory_type(memReqs.memoryTypeBits,
						   core::gfx::conversion::to_vk(m_BufferDataHandle->memoryPropertyFlags()),
						   &memAllocInfo.memoryTypeIndex);
	core::utility::vulkan::check(m_Context->device().allocateMemory(&memAllocInfo, nullptr, &m_Memory));

	core::utility::vulkan::check(m_Context->device().bindBufferMemory(m_Buffer, m_Memory, 0));
	m_Descriptor.buffer = m_Buffer;
	m_Descriptor.offset = 0;
	m_Descriptor.range	= m_BufferDataHandle->size();

	vk::FenceCreateInfo fi;
	fi.flags = vk::FenceCreateFlagBits::eSignaled;
	if(auto res = m_Context->device().createFence(&fi, nullptr, &m_BufferCompleted);
	   !core::utility::vulkan::check(res)) {
		core::ivk::log->critical(
		  "could not create a fence for an ivk::buffer_t [UID: {}] reason: {}.", m_UID.to_string(), vk::to_string(res));
		std::abort();
	}

	if(m_BufferDataHandle->region().allocator()->is_physically_backed()) {
		for(const memory::segment& it : m_BufferDataHandle->segments()) {
			map((std::byte*)it.range().begin, it.range().size(), 0);
		}
	}

	if(conversion::to_vk(m_BufferDataHandle->memoryPropertyFlags()) == vk::MemoryPropertyFlagBits::eDeviceLocal ||
	   conversion::to_vk(m_BufferDataHandle->usage()) & vk::BufferUsageFlagBits::eTransferDst) {
		vk::CommandBufferAllocateInfo cmdBufInfo;
		cmdBufInfo.commandPool = m_Context->command_pool();

		cmdBufInfo.level			  = vk::CommandBufferLevel::ePrimary;
		cmdBufInfo.commandBufferCount = 1;

		core::utility::vulkan::check(m_Context->device().allocateCommandBuffers(&cmdBufInfo, &m_CommandBuffer));
	}
}

buffer_t::~buffer_t() {
	ZoneScoped;
	core::ivk::log->info("destroying an ivk::buffer_t of {0} bytes size.", m_BufferDataHandle->size());
	m_Context->device().destroyBuffer(m_Buffer, nullptr);
	m_Context->device().freeMemory(m_Memory, nullptr);
	m_Context->device().destroyFence(m_BufferCompleted);
	if(conversion::to_vk(m_BufferDataHandle->memoryPropertyFlags()) == vk::MemoryPropertyFlagBits::eDeviceLocal) {
		m_Context->device().freeCommandBuffers(m_Context->command_pool(), 1, &m_CommandBuffer);
	}
}

size_t buffer_t::free_size() const noexcept {
	auto available = m_BufferDataHandle->region().allocator()->available();
	return std::accumulate(std::next(std::begin(available)),
						   std::end(available),
						   available[0].size(),
						   [](size_t sum, const memory::range_t& element) { return sum + element.size(); });
}
std::optional<memory::segment> buffer_t::reserve(vk::DeviceSize size) {
	return m_BufferDataHandle->allocate(psl::narrow_cast<size_t>(size));
}

std::vector<std::pair<memory::segment, memory::range_t>> buffer_t::reserve(std::vector<vk::DeviceSize> sizes,
																		   bool optimize) {
	ZoneScoped;
	auto scoped_lock		 = std::scoped_lock(m_Mutex);
	vk::DeviceSize totalSize = std::accumulate(
	  std::next(std::begin(sizes)), std::end(sizes), sizes[0], [](vk::DeviceSize sum, const vk::DeviceSize& element) {
		  return sum + element;
	  });
	std::vector<std::pair<memory::segment, memory::range_t>> result;


	psl_assert(totalSize <= std::numeric_limits<size_t>::max(),
			   "size should be lower than the numerical limits of the bitsize of the platform, should be less-equal "
			   "than '{}' but got '{}'",
			   std::numeric_limits<size_t>::max(),
			   totalSize);

	size_t totalSize_platform = (size_t)totalSize;

	// todo: low priority
	// this should check for biggest continuous space in the memory::region and split like that
	if(optimize) {
		auto segment = m_BufferDataHandle->allocate(totalSize_platform);
		if(segment) {
			result.resize(sizes.size());
			size_t accOffset = 0u;
			for(auto i = 0u; i < sizes.size(); ++i) {
				result[i].first	 = segment.value();
				result[i].second = memory::range_t {accOffset, accOffset + (size_t)sizes[i]};
				accOffset += (size_t)sizes[i];
			}
			return result;
		}
	}

	// either optimization failed, or we aren't optimizing
	result.reserve(sizes.size());
	for(const auto& size : sizes) {
		// has already been verified at the entry when calculating the total space requirement.
		size_t size_platform = (size_t)size;
		if(auto segment = m_BufferDataHandle->allocate(size_platform); segment) {
			auto& res  = result.emplace_back();
			res.first  = segment.value();
			res.second = memory::range_t {0, size_platform};
		} else {
			goto failure;
		}
	}

	return result;

	// in case something goes bad, roll back the already completed allocations
failure:
	for(auto& segment : result) {
		m_BufferDataHandle->deallocate(segment.first);
	}
	return {};
}

bool buffer_t::commit(std::vector<core::gfx::commit_instruction> instructions) {
	ZoneScoped;
	auto totalSize = std::accumulate(std::next(std::begin(instructions)),
									 std::end(instructions),
									 std::begin(instructions)->size,
									 [](auto sum, const commit_instruction& element) { return sum + element.size; });


	psl_assert(totalSize <= std::numeric_limits<size_t>::max(),
			   "size should be lower than the numerical limits of the bitsize of the platform, should be less-equal "
			   "than '{}' but got '{}'",
			   std::numeric_limits<size_t>::max(),
			   totalSize);

	size_t totalSize_platform = (size_t)totalSize;

	if(m_BufferDataHandle->memoryPropertyFlags() & core::gfx::memory_property::device_local) {
		std::vector<vk::DeviceSize> sizeRequests;
		sizeRequests.reserve(instructions.size());
		for(const auto& instruction : instructions) sizeRequests.emplace_back(instruction.size);

		auto stagingBuffer = m_StagingBuffer;
		if(!stagingBuffer) {
			core::ivk::log->warn("inefficient loading, dynamically creating a staging ivk::buffer_t.");
			memory::region temp_region {totalSize_platform, 4, new memory::default_allocator(false)};
			auto buffer_data = m_Cache.create<core::data::buffer_t>(core::gfx::memory_usage::transfer_source,
																	core::gfx::memory_property::host_visible |
																	  core::gfx::memory_property::host_coherent,
																	std::move(temp_region));

			stagingBuffer = m_Cache.create<core::ivk::buffer_t>(m_Context, buffer_data);
		}
		if(!stagingBuffer) {
			core::ivk::log->error("could not create an ivk::buffer_t for staging.");
			return false;
		}

		auto stagingSegments = stagingBuffer->reserve(sizeRequests, true);
		if(stagingSegments.size() == 0) {
			core::ivk::log->error("could not reserve the requested size in the staging buffer.");
			return false;
		}

		if(stagingSegments.size() > 0 && stagingSegments[0].first.range().size() == 0)
			debug_break();

		// put the data onto the staging buffer && create the copy region instructions
		std::vector<vk::BufferCopy> copyRegions;
		copyRegions.reserve(stagingSegments.size());

		auto boundSegment = stagingSegments[0].first;
		std::uintptr_t offset =
		  boundSegment.range().begin - (std::uintptr_t)stagingBuffer->m_BufferDataHandle->region().data();
		auto tuple = m_Context->device().mapMemory(stagingBuffer->m_Memory, offset, boundSegment.range().size());

		if(stagingSegments.size() > 0 && stagingSegments[0].first.range().size() == 0) {
			debug_break();
		}
		for(size_t i = 0; i < stagingSegments.size(); ++i) {
			if(stagingSegments[i].first.range() != boundSegment.range()) {
				m_Context->device().unmapMemory(stagingBuffer->m_Memory);
				boundSegment = stagingSegments[i].first;
				offset =
				  boundSegment.range().begin - (std::uintptr_t)stagingBuffer->m_BufferDataHandle->region().data();
				tuple = m_Context->device().mapMemory(stagingBuffer->m_Memory, offset, boundSegment.range().size());
			}

			if(!core::utility::vulkan::check(tuple.result)) {
				return false;
			}

			memcpy((void*)((std::uintptr_t)tuple.value + stagingSegments[i].second.begin),
				   (void*)(instructions[i].source),
				   instructions[i].size);

			if(m_BufferDataHandle->region().allocator()->is_physically_backed()) {
				memcpy((void*)(instructions[i].segment.range().begin +
							   instructions[i].sub_range.value_or(memory::range_t {}).begin),
					   (void*)(instructions[i].source),
					   instructions[i].size);
			}

			// todo we can collapse these regions and so invoke less buffercopy commands
			vk::BufferCopy& copyRegion = copyRegions.emplace_back();
			copyRegion.srcOffset	   = offset + stagingSegments[i].second.begin;
			copyRegion.dstOffset	   = instructions[i].segment.range().begin +
								   instructions[i].sub_range.value_or(memory::range_t {}).begin -
								   (std::uintptr_t)m_BufferDataHandle->region().data();
			copyRegion.size = instructions[i].size;

			if(stagingSegments.size() > 0 && stagingSegments[0].first.range().size() == 0) {
				debug_break();
			}
		}

		if(stagingSegments.size() > 0 && stagingSegments[0].first.range().size() == 0) {
			debug_break();
		}
		m_Context->device().unmapMemory(stagingBuffer->m_Memory);
		if(stagingBuffer != m_StagingBuffer) {
			auto res = copy_from(stagingBuffer.value(), copyRegions);
			for(auto segm : stagingSegments) {
				if(segm.second.begin == 0)
					stagingBuffer->deallocate(segm.first);
			}
			return res;
		} else {
			return copy_from_mt(stagingBuffer.value(), copyRegions, [stagingSegments, stagingBuffer]() mutable {
				for(auto segm : stagingSegments) {
					if(segm.second.begin == 0)
						stagingBuffer->deallocate(segm.first);
				}
			});
		}
		return true;
	} else {
		// core::ivk::log->info("mapping {0} regions into an ivk::buffer_t from CPU.", instructions.size());
		for(auto& instruction : instructions) {
			std::uintptr_t offset = instruction.segment.range().begin -
									(std::uintptr_t)m_BufferDataHandle->region().data() +
									instruction.sub_range.value_or(memory::range_t {}).begin;

			auto tuple = m_Context->device().mapMemory(m_Memory, offset, instruction.size);
			if(!core::utility::vulkan::check(tuple.result)) {
				return false;
			}
			memcpy(tuple.value, (void*)instruction.source, instruction.size);
			if(m_BufferDataHandle->region().allocator()->is_physically_backed()) {
				memcpy((void*)instruction.segment.range().begin, (void*)instruction.source, instruction.size);
			}
			m_Context->device().unmapMemory(m_Memory);
		}
	}
	return true;
}

bool buffer_t::deallocate(memory::segment& segment) {
	auto scoped_lock = std::scoped_lock(m_Mutex);
	return m_BufferDataHandle->deallocate(segment);
}

bool buffer_t::map(const void* data, vk::DeviceSize size, vk::DeviceSize offset) {
	ZoneScoped;
	if(size == 0) {
		return true;
	}

	psl_assert(size <= std::numeric_limits<size_t>::max(),
			   "size should be lower than the numerical limits of the bitsize of the platform, should be less-equal "
			   "than '{}' but got '{}'",
			   std::numeric_limits<size_t>::max(),
			   size);


	psl_assert(size + offset <= std::numeric_limits<size_t>::max(),
			   "size + offset should be lower than the numerical limits of the bitsize of the platform, should be "
			   "less-equal than '{}' but got '{}'",
			   std::numeric_limits<size_t>::max(),
			   size + offset);

	size_t platform_size   = psl::narrow_cast<size_t>(size);
	size_t platform_offset = psl::narrow_cast<size_t>(offset);

	if(platform_size > m_BufferDataHandle->size() || (platform_size + platform_offset) > m_BufferDataHandle->size()) {
		core::ivk::log->error("tried to map an incorrect size amount to an ivk::buffer_t.");
		return false;
	}

	if(m_BufferDataHandle->memoryPropertyFlags() & core::gfx::memory_property::device_local) {
		if(m_StagingBuffer) {
			m_StagingBuffer->map(data, size, 0);
			return copy_from(m_StagingBuffer.value(), {vk::BufferCopy {0u, offset, size}});
		} else {
			// make a local staging buffer, this is hardly efficient. todo find better way.
			core::ivk::log->warn("inefficient loading, dynamically creating a staging ivk::buffer_t.");
			memory::region temp_region {platform_size * 2, 4, new memory::default_allocator(true)};
			auto buffer_data = m_Cache.create<core::data::buffer_t>(core::gfx::memory_usage::transfer_source,
																	core::gfx::memory_property::host_visible |
																	  core::gfx::memory_property::host_coherent,
																	std::move(temp_region));
			auto staging	 = m_Cache.create<core::ivk::buffer_t>(m_Context, buffer_data);

			auto tuple = m_Context->device().mapMemory(staging->m_Memory, 0, size);
			if(!core::utility::vulkan::check(tuple.result)) {
				debug_break();
			}
			memcpy(tuple.value, data, platform_size);
			m_Context->device().unmapMemory(staging->m_Memory);

			return copy_from(staging.value(), {vk::BufferCopy {0u, offset, size}});
		}
	} else {
		// core::ivk::log->info("mapping ivk::buffer_t data from CPU.");

		auto tuple = m_Context->device().mapMemory(m_Memory, offset, size);
		if(!core::utility::vulkan::check(tuple.result)) {
			debug_break();
		}
		memcpy(tuple.value, data, platform_size);
		if(m_BufferDataHandle->region().data() != data)	   // Let's map this to the buffer too.
			memcpy(m_BufferDataHandle->region().data(), data, platform_size);

		m_Context->device().unmapMemory(m_Memory);
		return true;
	}
}
// bool buffer_t::map(const memory::region& region, const memory::segment& segment)
//{
//	return map((void*)(segment.range().begin + (std::uintptr_t)region.data()), segment.range().size(), 0);
//}
// bool buffer_t::map(const memory::region& region, const memory::segment& segment, const memory::range_t& sub)
//{
//	return map((void*)(sub.begin + (std::uintptr_t)region.data()), sub.size(), 0);
//}
bool buffer_t::copy_from_mt(const buffer_t& other,
							const std::vector<vk::BufferCopy>& copyRegions,
							std::function<void()> on_finish) {
	ZoneScoped;

	auto totalsize =
	  std::accumulate(copyRegions.begin(), copyRegions.end(), 0, [&](int sum, const vk::BufferCopy& region) {
		  return sum + (int)region.size;
	  });

	core::ivk::log->debug("copying buffer {0} into {1} for a total size of {2} using {3} copy instructions",
						  psl::utility::to_string(other.m_UID),
						  psl::utility::to_string(m_UID),
						  totalsize,
						  copyRegions.size());

	copy_from_manager.schedule(other, *this, copyRegions, on_finish);
	copy_from_manager.execute(m_Context->transfer_queue(), m_CommandBuffer, m_BufferCompleted, m_Context->device());
	return true;
}

bool buffer_t::copy_from(const buffer_t& other, const std::vector<vk::BufferCopy>& copyRegions) {
	ZoneScoped;
	vk::Queue queue = m_Context->transfer_queue();
	wait_until_ready();

	auto totalsize =
	  std::accumulate(copyRegions.begin(), copyRegions.end(), 0, [&](int sum, const vk::BufferCopy& region) {
		  return sum + (int)region.size;
	  });

	core::ivk::log->debug("copying buffer {0} into {1} for a total size of {2} using {3} copy instructions",
						  psl::utility::to_string(other.m_UID),
						  psl::utility::to_string(m_UID),
						  totalsize,
						  copyRegions.size());

	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.pNext = NULL;

	{
		auto scoped_lock = std::scoped_lock(m_Mutex);
		// Put buffer region copies into command buffer
		// Note that the staging buffer must not be deleted before the copies
		// have been submitted and executed
		core::utility::vulkan::check(m_CommandBuffer.begin(&cmdBufferBeginInfo));
		m_CommandBuffer.copyBuffer(other.m_Buffer, m_Buffer, (uint32_t)copyRegions.size(), copyRegions.data());
		if(auto res = m_CommandBuffer.end(); !core::utility::vulkan::check(res)) {
			core::ivk::log->critical("could not end the command buffer for an ivk::buffer_t copy operation. Reason: {}",
									 vk::to_string(res));
			std::abort();
		}

		// Submit copies to the queue
		vk::SubmitInfo copySubmitInfo;
		copySubmitInfo.commandBufferCount = 1;
		copySubmitInfo.pCommandBuffers	  = &m_CommandBuffer;
		m_Context->device().resetFences(m_BufferCompleted);
		core::utility::vulkan::check(queue.submit(1, &copySubmitInfo, m_BufferCompleted));
		if(auto res = queue.waitIdle(); !core::utility::vulkan::check(res)) {
			core::ivk::log->critical(
			  "could not wait for the queue to become idle after an ivk::buffer_t copy operation. "
			  "Reason: {}",
			  vk::to_string(res));
			std::abort();
		}
	}

	if(m_BufferDataHandle->memoryPropertyFlags() & core::gfx::memory_property::host_visible) {
		// TODO this really needs to be per region..
		// auto tuple = m_Context->Device().mapMemory(m_Memory, 0, m_Descriptor.range);
		// core::utility::vulkan::check(tuple.result);
		// memcpy(m_BufferData.Data(), tuple.value, m_Descriptor.range);
		// m_Context->Device().unmapMemory(m_Memory);

		core::ivk::log->info("mapping an ivk::buffer_t of size {0} to a pool of size {1}",
							 psl::utility::to_string(m_BufferDataHandle->size()),
							 psl::utility::to_string(m_BufferDataHandle->size()));

		uint32_t minVal {std::numeric_limits<uint32_t>::max()}, maxVal {std::numeric_limits<uint32_t>::min()};
		for(const auto& region : copyRegions) {
			minVal = std::min((uint32_t)region.dstOffset, minVal);
			maxVal = std::max((uint32_t)(region.dstOffset + region.size), maxVal);
		}
		core::ivk::log->info("copy range (start|finish): {0} | {1}", minVal, maxVal);

		if(maxVal > m_BufferDataHandle->size()) {
			core::ivk::log->error("range exceeds the allocated ivk::buffer_t size! (src|dst) {0} | {1}",
								  m_BufferDataHandle->size(),
								  maxVal);
			debug_break();
		}

		for(const auto& region : copyRegions) {
			auto tuple = m_Context->device().mapMemory(m_Memory, region.dstOffset, region.size);
			core::ivk::log->info("(dstOffset|size) {0} | {1}", region.dstOffset, region.size);
			if(core::utility::vulkan::check(tuple.result)) {
				if(auto segment = m_BufferDataHandle->allocate(psl::narrow_cast<size_t>(region.size)); segment) {
					memcpy((void*)(segment.value().range().begin), tuple.value, psl::narrow_cast<size_t>(region.size));
				} else {
					core::ivk::log->error("could not get a free spot in the pool for some reason..");
					debug_break();
				}
			}
			m_Context->device().unmapMemory(m_Memory);
		}
		return true;
	}

	// m_CommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	return true;
}


bool buffer_t::set(const void* data,
				   std::vector<vk::BufferCopy> commands)	// maps to the UpdateBuffer of the old version
{
	ZoneScoped;
	if(data == nullptr || commands.size() == 0) {
		core::ivk::log->error(((data == nullptr)
								 ? "tried passing nullptr to update an ivk::buffer_t"
								 : "tried updating an ivk::buffer_t, but forgetting to send commands."));
		return false;
	}

	wait_until_ready();
	auto end = std::remove_if(std::begin(commands), std::end(commands), [](const vk::BufferCopy& c) {
		return c.size == 0 || c.dstOffset + c.size >= max_size_set;
	});
	if(end == std::begin(commands) || end != std::end(commands)) {
		core::ivk::log->error(((end == std::begin(commands))
								 ? "after removing all invalid commands, there were no suitable instructions left."
								 : "several (but not all) commands were invalid."));

		psl::string8_t message = "\n";
		for(auto it = end; it != std::end(commands); ++it)
			message += "size: " + psl::utility::to_string(it->size) +
					   " srcOffset: " + psl::utility::to_string(it->srcOffset) +
					   " dstOffset: " + psl::utility::to_string(it->dstOffset) + "\n";
		core::ivk::log->debug(message);
		return false;
	}

	vk::Queue queue = m_Context->transfer_queue();

	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.pNext = nullptr;


	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies
	// have been submitted and executed
	core::utility::vulkan::check(m_CommandBuffer.begin(&cmdBufferBeginInfo));

	// now we will do all the seperate commands.
	for(auto it = std::begin(commands); it != end; ++it) {
		m_CommandBuffer.updateBuffer(
		  m_Buffer, it->dstOffset, it->size, (uint32_t*)(static_cast<const char*>(data) + it->srcOffset));
	}
	if(auto res = m_CommandBuffer.end(); !core::utility::vulkan::check(res)) {
		core::ivk::log->critical("could not end the command buffer for an ivk::buffer_t update operation. Reason: {}",
								 vk::to_string(res));
		std::abort();
	}

	// Submit copies to the queue
	vk::SubmitInfo copySubmitInfo;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers	  = &m_CommandBuffer;

	m_Context->device().resetFences(m_BufferCompleted);
	core::utility::vulkan::check(queue.submit(1, &copySubmitInfo, m_BufferCompleted));
	// queue.waitIdle();
	// m_CommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

	return true;
}
bool buffer_t::is_busy() const {
	return m_Context->device().getFenceStatus(m_BufferCompleted) != vk::Result::eSuccess;
}

void buffer_t::wait_until_ready(uint64_t timeout) const {
	ZoneScoped;
	if(is_busy()) {
		if(auto res = m_Context->device().waitForFences(m_BufferCompleted, VK_TRUE, timeout);
		   !core::utility::vulkan::check(res)) {
			core::ivk::log->critical(
			  "could not wait for the fence of an ivk::buffer_t [UID: {}] to become ready. "
			  "Reason: {}",
			  m_UID.to_string(),
			  vk::to_string(res));
			std::abort();
		}
	}
}

const vk::Buffer& buffer_t::gpu_buffer() const {
	return m_Buffer;
}
core::resource::handle<core::data::buffer_t> buffer_t::data() const {
	return m_BufferDataHandle;
}
vk::DescriptorBufferInfo& buffer_t::buffer_info() {
	return m_Descriptor;
}
