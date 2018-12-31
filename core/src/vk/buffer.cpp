#include "stdafx.h"
#include "vk/buffer.h"
#include "data/buffer.h"
#include "vk/context.h"

using namespace psl;
using namespace core;
using namespace core::gfx;
using namespace core::resource;


// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkCmdUpdateBuffer.html
static const size_t max_size_set{65535};

buffer::buffer(const UID& uid, cache& cache, handle<context> context, handle<data::buffer> buffer_data,
			   std::optional<core::resource::handle<core::gfx::buffer>> staging_buffer)
	: m_Context(context), m_BufferDataHandle(std::move(buffer_data)), m_Cache(cache), m_UID(uid),
	  m_StagingBuffer(staging_buffer.value_or(core::resource::handle<core::gfx::buffer>{}))
{
	PROFILE_SCOPE(core::profiler)
	core::ivk::log->info("creating an ivk::buffer of {0} bytes size.", m_BufferDataHandle->size());
	vk::MemoryRequirements memReqs;
	auto& region   = m_BufferDataHandle->region();
	auto alignment = region.alignment();
	auto type	  = m_BufferDataHandle->usage();

	if(type & vk::BufferUsageFlagBits::eUniformBuffer &&
	   alignment != m_Context->properties().limits.minUniformBufferOffsetAlignment)
	{
		core::ivk::log->warn(
			"trying to create an ivk::buffer [UID: {0} ] with incorrect alignment, alignment is: {1}, but should be: "
			"{2}",
			uid.to_string(), alignment, m_Context->properties().limits.minUniformBufferOffsetAlignment);
	}
	else if(type & vk::BufferUsageFlagBits::eStorageBuffer &&
			alignment != m_Context->properties().limits.minStorageBufferOffsetAlignment)
	{
		core::ivk::log->warn(
			"trying to create an ivk::buffer [UID: {0} ] with incorrect alignment, alignment is: {1}, but should be: "
			"{2}",
			uid.to_string(), alignment, m_Context->properties().limits.minStorageBufferOffsetAlignment);
	}

	vk::MemoryAllocateInfo memAllocInfo;
	memAllocInfo.pNext			 = NULL;
	memAllocInfo.allocationSize  = 0;
	memAllocInfo.memoryTypeIndex = 0;

	// Vertex buffer
	vk::BufferCreateInfo bufCreateInfo;
	bufCreateInfo.pNext = NULL;
	bufCreateInfo.usage = m_BufferDataHandle->usage();
	bufCreateInfo.size  = m_BufferDataHandle->size();
	bufCreateInfo.flags = vk::BufferCreateFlagBits();

	utility::vulkan::check(m_Context->device().createBuffer(&bufCreateInfo, nullptr, &m_Buffer));

	memReqs = m_Context->device().getBufferMemoryRequirements(m_Buffer), memAllocInfo.allocationSize = memReqs.size;

	m_Context->memory_type(memReqs.memoryTypeBits, m_BufferDataHandle->memoryPropertyFlags(),
						   &memAllocInfo.memoryTypeIndex);
	utility::vulkan::check(m_Context->device().allocateMemory(&memAllocInfo, nullptr, &m_Memory));

	utility::vulkan::check(m_Context->device().bindBufferMemory(m_Buffer, m_Memory, 0));
	m_Descriptor.buffer = m_Buffer;
	m_Descriptor.offset = 0;
	m_Descriptor.range  = m_BufferDataHandle->size();

	vk::FenceCreateInfo fi;
	fi.flags = vk::FenceCreateFlagBits::eSignaled;
	m_Context->device().createFence(&fi, nullptr, &m_BufferCompleted);

	if(m_BufferDataHandle->region().allocator()->is_physically_backed())
	{
		for(const memory::segment& it : m_BufferDataHandle->segments())
		{
			map((std::byte*)it.range().begin, it.range().size(), 0);
		}
	}

	if(m_BufferDataHandle->memoryPropertyFlags() == vk::MemoryPropertyFlagBits::eDeviceLocal ||
	   m_BufferDataHandle->usage() & vk::BufferUsageFlagBits::eTransferDst)
	{
		vk::CommandBufferAllocateInfo cmdBufInfo;
		cmdBufInfo.commandPool = m_Context->command_pool();

		cmdBufInfo.level			  = vk::CommandBufferLevel::ePrimary;
		cmdBufInfo.commandBufferCount = 1;

		utility::vulkan::check(m_Context->device().allocateCommandBuffers(&cmdBufInfo, &m_CommandBuffer));
	}
}

buffer::~buffer()
{
	PROFILE_SCOPE(core::profiler)
	core::ivk::log->info("destroying an ivk::buffer of {0} bytes size.", m_BufferDataHandle->size());
	m_Context->device().destroyBuffer(m_Buffer, nullptr);
	m_Context->device().freeMemory(m_Memory, nullptr);
	m_Context->device().destroyFence(m_BufferCompleted);
	if(m_BufferDataHandle->memoryPropertyFlags() == vk::MemoryPropertyFlagBits::eDeviceLocal)
	{
		m_Context->device().freeCommandBuffers(m_Context->command_pool(), 1, &m_CommandBuffer);
	}
}

size_t buffer::free_size() const noexcept
{
	auto available = m_BufferDataHandle->region().allocator()->available();
	return std::accumulate(std::next(std::begin(available)), std::end(available), available[0].size(),
					[](size_t sum, const memory::range& element) { return sum + element.size(); });
}
std::optional<memory::segment> buffer::reserve(vk::DeviceSize size) { return m_BufferDataHandle->allocate(size); }

std::vector<std::pair<memory::segment, memory::range>> buffer::reserve(std::vector<vk::DeviceSize> sizes, bool optimize)
{
	PROFILE_SCOPE(core::profiler)
	vk::DeviceSize totalSize =
		std::accumulate(std::next(std::begin(sizes)), std::end(sizes), sizes[0],
						[](vk::DeviceSize sum, const vk::DeviceSize& element) { return sum + element; });
	std::vector<std::pair<memory::segment, memory::range>> result;

	// todo: low priority
	// this should check for biggest continuous space in the memory::region and split like that
	if(optimize)
	{
		auto segment = m_BufferDataHandle->allocate(totalSize);
		if(segment)
		{
			result.resize(sizes.size());
			vk::DeviceSize accOffset = 0u;
			for(auto i = 0u; i < sizes.size(); ++i)
			{
				result[i].first  = segment.value();
				result[i].second = memory::range{accOffset, accOffset + sizes[i]};
				accOffset += sizes[i];
			}
			return result;
		}
	}

	// either optimization failed, or we aren't optimizing
	result.reserve(sizes.size());
	for(const auto& size : sizes)
	{
		if(auto segment = m_BufferDataHandle->allocate(size); segment)
		{
			auto& res = result.emplace_back();
			res.first = segment.value();
			res.second = memory::range{0, size};
		}
		else
		{
			goto failure;
		}
	}

	return result;

	// in case something goes bad, roll back the already completed allocations
failure:
	for(auto& segment : result)
	{
		m_BufferDataHandle->deallocate(segment.first);
	}
	return {};
}

bool buffer::commit(std::vector<commit_instruction> instructions)
{
	PROFILE_SCOPE(core::profiler)
	vk::DeviceSize totalSize =
		std::accumulate(std::next(std::begin(instructions)), std::end(instructions), std::begin(instructions)->size,
						[](vk::DeviceSize sum, const commit_instruction& element) { return sum + element.size; });

	if(m_BufferDataHandle->memoryPropertyFlags() & vk::MemoryPropertyFlagBits::eDeviceLocal)
	{
		std::vector<vk::DeviceSize> sizeRequests;
		sizeRequests.reserve(instructions.size());
		for(const auto& instruction : instructions)
			sizeRequests.emplace_back(instruction.size);

		auto stagingBuffer = m_StagingBuffer;
		if(!stagingBuffer)
		{
			core::ivk::log->warn("inefficient loading, dynamically creating a staging ivk::buffer.");
			stagingBuffer = core::resource::create<core::gfx::buffer>(m_Cache);
			auto buffer_data = core::resource::create<core::data::buffer>(m_Cache);

			memory::region temp_region{totalSize, 4, new memory::default_allocator(false)};
			buffer_data.load(vk::BufferUsageFlagBits::eTransferSrc,
							 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
							 std::move(temp_region));
			stagingBuffer.load(m_Context, buffer_data);
		}
		if(!stagingBuffer)
		{
			core::ivk::log->error("could not create an ivk::buffer for staging.");
			return false;
		}

		auto stagingSegments = stagingBuffer->reserve(sizeRequests, true);
		if(stagingSegments.size() == 0)
		{
			core::ivk::log->error("could not reserve the requested size in the staging buffer.");
			return false;
		}

		if (stagingSegments.size() > 0 && stagingSegments[0].first.range().size() == 0)
			debug_break();

		// put the data onto the staging buffer && create the copy region instructions
		std::vector<vk::BufferCopy> copyRegions;
		copyRegions.reserve(stagingSegments.size());

		auto boundSegment = stagingSegments[0].first;
		std::uintptr_t offset = boundSegment.range().begin - (std::uintptr_t)stagingBuffer->m_BufferDataHandle->region().data();
		auto tuple = m_Context->device().mapMemory(stagingBuffer->m_Memory, offset, boundSegment.range().size());

		if (stagingSegments.size() > 0 && stagingSegments[0].first.range().size() == 0)
			debug_break();
		for(auto i = 0; i < stagingSegments.size(); ++i)
		{
			if(stagingSegments[i].first.range() != boundSegment.range())
			{
				m_Context->device().unmapMemory(stagingBuffer->m_Memory);
				boundSegment = stagingSegments[i].first;
				offset = boundSegment.range().begin - (std::uintptr_t)stagingBuffer->m_BufferDataHandle->region().data();
				tuple = m_Context->device().mapMemory(stagingBuffer->m_Memory, offset, boundSegment.range().size());
			}

			if(!utility::vulkan::check(tuple.result))
			{
				return false;
			}

			memcpy((void*)((std::uintptr_t)tuple.value + stagingSegments[i].second.begin), (void*)(instructions[i].source), instructions[i].size);

			if(m_BufferDataHandle->region().allocator()->is_physically_backed())
			{
				memcpy((void*)(instructions[i].segment.range().begin + instructions[i].sub_range.value_or(memory::range{}).begin), (void*)(instructions[i].source), instructions[i].size);
			}

			// todo we can collapse these regions and so invoke less buffercopy commands
			vk::BufferCopy& copyRegion = copyRegions.emplace_back();
			copyRegion.srcOffset = offset + stagingSegments[i].second.begin;
			copyRegion.dstOffset = instructions[i].segment.range().begin + instructions[i].sub_range.value_or(memory::range{}).begin - (std::uintptr_t)m_BufferDataHandle->region().data();
			copyRegion.size = instructions[i].size;

			if (stagingSegments.size() > 0 && stagingSegments[0].first.range().size() == 0)
				debug_break();
		}

		if (stagingSegments.size() > 0 && stagingSegments[0].first.range().size() == 0)
			debug_break();
		m_Context->device().unmapMemory(stagingBuffer->m_Memory);
		auto res = copy_from(stagingBuffer, copyRegions);
		for(auto segm : stagingSegments)
		{
			if(segm.second.begin == 0)
				stagingBuffer->deallocate(segm.first);
		}
		return res;
	}
	else
	{
		//core::ivk::log->info("mapping {0} regions into an ivk::buffer from CPU.", instructions.size());
		for(auto& alloc_segm : instructions)
		{
			std::uintptr_t offset = alloc_segm.segment.range().begin -
				(std::uintptr_t)m_BufferDataHandle->region().data() + alloc_segm.sub_range.value_or(memory::range{}).begin;

			auto tuple = m_Context->device().mapMemory(m_Memory, offset, alloc_segm.size);
			if(!utility::vulkan::check(tuple.result))
			{
				return false;
			}
			memcpy(tuple.value, (void*)alloc_segm.source, alloc_segm.size);
			if(m_BufferDataHandle->region().allocator()->is_physically_backed())
			{
				memcpy((void*)alloc_segm.segment.range().begin, (void*)alloc_segm.source, alloc_segm.size);
			}
			m_Context->device().unmapMemory(m_Memory);
		}
	}
	return true;
}

bool buffer::deallocate(memory::segment& segment) { return m_BufferDataHandle->deallocate(segment); }

bool buffer::map(const void* data, vk::DeviceSize size, vk::DeviceSize offset)
{
	PROFILE_SCOPE(core::profiler)
	if(size == 0) return true;

	if(size > m_BufferDataHandle->size() || (size + offset) > m_BufferDataHandle->size())
	{
		core::ivk::log->error("tried to map an incorrect size amount to an ivk::buffer.");
		return false;
	}

	if(m_BufferDataHandle->memoryPropertyFlags() == vk::MemoryPropertyFlagBits::eDeviceLocal)
	{
		if(m_StagingBuffer)
		{
			m_StagingBuffer->map(data, size, 0);
			return copy_from(m_StagingBuffer, {vk::BufferCopy{0u, offset, size}});
		}
		else
		{
			// make a local staging buffer, this is hardly efficient. todo find better way.
			core::ivk::log->warn("inefficient loading, dynamically creating a staging ivk::buffer.");
			auto staging	 = core::resource::create<core::gfx::buffer>(m_Cache);
			auto buffer_data = core::resource::create<core::data::buffer>(m_Cache);

			memory::region temp_region{size * 2, 4, new memory::default_allocator(true)};
			buffer_data.load(vk::BufferUsageFlagBits::eTransferSrc,
							 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
							 std::move(temp_region));
			staging.load(m_Context, buffer_data);

			auto tuple = m_Context->device().mapMemory(staging->m_Memory, 0, size);
			if(!utility::vulkan::check(tuple.result))
			{
				debug_break();
			}
			memcpy(tuple.value, data, size);
			m_Context->device().unmapMemory(staging->m_Memory);

			return copy_from(staging, {vk::BufferCopy{0u, offset, size}});
		}
	}
	else
	{
		//core::ivk::log->info("mapping ivk::buffer data from CPU.");

		auto tuple = m_Context->device().mapMemory(m_Memory, offset, size);
		if(!utility::vulkan::check(tuple.result))
		{
			debug_break();
		}
		memcpy(tuple.value, data, size);
		if(m_BufferDataHandle->region().data() != data) // Let's map this to the buffer too.
			memcpy(m_BufferDataHandle->region().data(), data, size);

		m_Context->device().unmapMemory(m_Memory);
		return true;
	}
}
// bool buffer::map(const memory::region& region, const memory::segment& segment)
//{
//	return map((void*)(segment.range().begin + (std::uintptr_t)region.data()), segment.range().size(), 0);
//}
// bool buffer::map(const memory::region& region, const memory::segment& segment, const memory::range& sub)
//{
//	return map((void*)(sub.begin + (std::uintptr_t)region.data()), sub.size(), 0);
//}

bool buffer::copy_from(const core::resource::handle<buffer>& other, const std::vector<vk::BufferCopy>& copyRegions)
{
	PROFILE_SCOPE(core::profiler)
	core::profiler.scope_begin("prepare", this);
	vk::Queue queue = m_Context->queue();
	wait_until_ready();

	auto totalsize = std::accumulate(copyRegions.begin(), copyRegions.end(), 0,
									 [&](int sum, const vk::BufferCopy& region) { return sum + (int)region.size; });

	core::ivk::log->info("copying buffer {0} into {1} for a total size of {2} using {3} copy instructions",
		utility::to_string(other->m_UID), utility::to_string(m_UID), totalsize, copyRegions.size());

	for(const auto& region : copyRegions)
	{
		core::ivk::log->info("srcOffset | dstOffset | size : {0} | {1} | {2}", region.srcOffset, region.dstOffset,
			region.size);
	}

	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.pNext = NULL;

	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies
	// have been submitted and executed
	utility::vulkan::check(m_CommandBuffer.begin(&cmdBufferBeginInfo));
	m_CommandBuffer.copyBuffer(other->m_Buffer, m_Buffer, (uint32_t)copyRegions.size(), copyRegions.data());
	m_CommandBuffer.end();

	// Submit copies to the queue
	vk::SubmitInfo copySubmitInfo;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers	= &m_CommandBuffer;
	m_Context->device().resetFences(m_BufferCompleted);
	core::profiler.scope_end(this);
	utility::vulkan::check(queue.submit(1, &copySubmitInfo, m_BufferCompleted));
	core::profiler.scope_begin("wait idle", this);
	queue.waitIdle();
	core::profiler.scope_end(this);

	if(m_BufferDataHandle->memoryPropertyFlags() == vk::MemoryPropertyFlagBits::eHostVisible)
	{
		core::profiler.scope_begin("replicate to host", this);
		// TODO this really needs to be per region..
		// auto tuple = m_Context->Device().mapMemory(m_Memory, 0, m_Descriptor.range);
		// utility::vulkan::check(tuple.result);
		// memcpy(m_BufferData.Data(), tuple.value, m_Descriptor.range);
		// m_Context->Device().unmapMemory(m_Memory);

		core::ivk::log->info("mapping an ivk::buffer of size {0} to a pool of size {1}",
			utility::to_string(m_BufferDataHandle->size()), utility::to_string(m_BufferDataHandle->size()));

		uint32_t minVal{std::numeric_limits<uint32_t>::max()}, maxVal{std::numeric_limits<uint32_t>::min()};
		for(const auto& region : copyRegions)
		{
			minVal = std::min((uint32_t)region.dstOffset, minVal);
			maxVal = std::max((uint32_t)(region.dstOffset + region.size), maxVal);
		}
		core::ivk::log->info("copy range {start|finish}: {0} | {1}", minVal, maxVal);

		if(maxVal > m_BufferDataHandle->size())
		{
			core::ivk::log->error("range exceeds the allocated ivk::buffer size! {src|dst} {0} | {1}",
								  m_BufferDataHandle->size(), maxVal);
			debug_break();
		}

		for(const auto& region : copyRegions)
		{
			auto tuple = m_Context->device().mapMemory(m_Memory, region.dstOffset, region.size);
			core::ivk::log->info("{dstOffset|size} {0} | {1}", region.dstOffset, region.size);
			if(utility::vulkan::check(tuple.result))
			{
				if(auto segment = m_BufferDataHandle->allocate(region.size); segment)
				{
					memcpy((void*)(segment.value().range().begin), tuple.value, region.size);
				}
				else
				{
					core::ivk::log->error("could not get a free spot in the pool for some reason..");
					debug_break();
				}
			}
			m_Context->device().unmapMemory(m_Memory);
		}
		core::profiler.scope_end(this);
		return true;
	}

	// m_CommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	return true;
}


bool buffer::set(const void* data, std::vector<vk::BufferCopy> commands) // maps to the UpdateBuffer of the old version
{
	PROFILE_SCOPE(core::profiler)
	if(data == nullptr || commands.size() == 0)
	{
		core::ivk::log->error(((data == nullptr) ? "tried passing nullptr to update an ivk::buffer"
												 : "tried updating an ivk::buffer, but forgetting to send commands."));
		return false;
	}

	wait_until_ready();
	auto end = std::remove_if(std::begin(commands), std::end(commands), [](const vk::BufferCopy& c) {
		return c.size == 0 || c.dstOffset + c.size >= max_size_set;
	});
	if(end == std::begin(commands) || end != std::end(commands))
	{
		core::ivk::log->error(((end == std::begin(commands))
								   ? "after removing all invalid commands, there were no suitable instructions left."
								   : "several (but not all) commands were invalid."));

		psl::string8_t message = "\n";
		for(auto it = end; it != std::end(commands); ++it)
			message += "size: " + utility::to_string(it->size) + " srcOffset: " + utility::to_string(it->srcOffset) +
					   " dstOffset: " + utility::to_string(it->dstOffset) + "\n";
		core::ivk::log->debug(message);
		return false;
	}

	vk::Queue queue = m_Context->queue();

	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.pNext = nullptr;


	// Put buffer region copies into command buffer
	// Note that the staging buffer must not be deleted before the copies
	// have been submitted and executed
	utility::vulkan::check(m_CommandBuffer.begin(&cmdBufferBeginInfo));

	// now we will do all the seperate commands.
	for(auto it = std::begin(commands); it != end; ++it)
	{
		m_CommandBuffer.updateBuffer(m_Buffer, it->dstOffset, it->size,
									 (uint32_t*)(static_cast<const char*>(data) + it->srcOffset));
	}
	m_CommandBuffer.end();

	// Submit copies to the queue
	vk::SubmitInfo copySubmitInfo;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers	= &m_CommandBuffer;

	m_Context->device().resetFences(m_BufferCompleted);
	utility::vulkan::check(queue.submit(1, &copySubmitInfo, m_BufferCompleted));
	//queue.waitIdle();
	// m_CommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

	return true;
}
bool buffer::is_busy() const { return m_Context->device().getFenceStatus(m_BufferCompleted) != vk::Result::eSuccess; }

void buffer::wait_until_ready(uint64_t timeout) const
{
	PROFILE_SCOPE(core::profiler)
	if(is_busy())
	{
		m_Context->device().waitForFences(m_BufferCompleted, VK_TRUE, timeout);
	}
}

const vk::Buffer& buffer::gpu_buffer() const { return m_Buffer; }
core::resource::handle<core::data::buffer> buffer::data() const { return m_BufferDataHandle; }
vk::DescriptorBufferInfo& buffer::buffer_info() { return m_Descriptor; }
