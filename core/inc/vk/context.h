#pragma once
#include <optional>
#include "vulkan_stdafx.h"


namespace core::ivk
{
	/// \brief all known extensions for Vulkan.
	enum class extensions
	{
		KHR_8bit_storage,
		KHR_android_surface,
		KHR_create_renderpass2,
		KHR_display,
		KHR_display_swapchain,
		KHR_draw_indirect_count,
		KHR_external_fence_fd,
		KHR_external_fence_win32,
		KHR_external_memory_fd,
		KHR_external_memory_win32,
		KHR_external_semaphore_fd,
		KHR_external_semaphore_win32,
		KHR_get_display_properties2,
		KHR_get_surface_capabilities2,
		KHR_image_format_list,
		KHR_incremental_present,
		KHR_mir_surface,
		KHR_push_descriptor,
		KHR_sampler_mirror_clamp_to_edge,
		KHR_shared_presentable_image,
		KHR_surface,
		KHR_swapchain,
		KHR_wayland_surface,
		KHR_win32_keyed_mutex,
		KHR_win32_surface,
		KHR_xcb_surface,
		KHR_xlib_surface,
		EXT_acquire_xlib_display,
		EXT_blend_operation_advanced,
		EXT_conditional_rendering,
		EXT_conservative_rasterization,
		EXT_debug_utils,
		EXT_depth_range_unrestricted,
		EXT_descriptor_indexing,
		EXT_direct_mode_display,
		EXT_discard_rectangles,
		EXT_display_control,
		EXT_display_surface_counter,
		EXT_external_memory_dma_buf,
		EXT_external_memory_host,
		EXT_global_priority,
		EXT_hdr_metadata,
		EXT_post_depth_coverage,
		EXT_queue_family_foreign,
		EXT_sample_locations,
		EXT_sampler_filter_minmax,
		EXT_shader_stencil_export,
		EXT_shader_subgroup_ballot,
		EXT_shader_subgroup_vote,
		EXT_shader_viewport_index_layer,
		EXT_swapchain_colorspace,
		EXT_validation_cache,
		EXT_validation_flags,
		EXT_vertex_attribute_divisor,
		AMD_buffer_marker,
		AMD_gcn_shader,
		AMD_gpu_shader_half_float,
		AMD_gpu_shader_int16,
		AMD_mixed_attachment_samples,
		AMD_rasterization_order,
		AMD_shader_ballot,
		AMD_shader_core_properties,
		AMD_shader_explicit_vertex_parameter,
		AMD_shader_fragment_mask,
		AMD_shader_image_load_store_lod,
		AMD_shader_info,
		AMD_shader_trinary_minmax,
		AMD_texture_gather_bias_lod,
		ANDROID_external_memory_android_hardware_buffer,
		GOOGLE_display_timing,
		IMG_filter_cubic,
		MVK_ios_surface,
		MVK_macos_surface,
		NN_vi_surface,
		NVX_device_generated_commands,
		NVX_multiview_per_view_attributes,
		NV_clip_space_w_scaling,
		NV_device_diagnostic_checkpoints,
		NV_fill_rectangle,
		NV_fragment_coverage_to_color,
		NV_framebuffer_mixed_samples,
		NV_geometry_shader_passthrough,
		NV_sample_mask_override_coverage,
		NV_shader_subgroup_partitioned,
		NV_viewport_array2,
		NV_viewport_swizzle
	};
} // namespace core::ivk

namespace psl
{
	struct UID;
}

namespace core::os
{
	class surface;
}

namespace core::resource
{
	class cache;
}
namespace core::ivk
{
	/// \brief encapsulated a graphics context.
	///
	/// a context encapsulated a physical device (GPU), as well as a instance of the graphics context on the driver.
	/// it is the single point of information that many operations need in Vulkan, so that those operations know
	/// who will be executing them. As such, the context class is fundamental, and will be passed around a lot.
	/// It is often one of the first objects to be created, and one of the last to be destroyed in a most applications.
	class context
	{
	  public:
		template <typename T>
		using optional_ref = std::optional<std::reference_wrapper<T>>;

		context(const psl::UID &uid, core::resource::cache &cache, psl::string8::view name, uint32_t deviceIndex = 0u);
		~context();
		context(const context &) = delete;
		context(context &&)		 = delete;
		context &operator=(const context &) = delete;
		context &operator=(context &&) = delete;

		/// \returns the vk::Instance of this context.
		const vk::Instance &instance() const noexcept;
		/// \returns the vk::Device of this context.
		const vk::Device &device() const noexcept;
		/// \returns the vk::PhysicalDevice of this context.
		const vk::PhysicalDevice &physical_device() const noexcept;

		/// \returns the physical device properties (for e.g. checking device limits)
		const vk::PhysicalDeviceProperties &properties() const noexcept;
		/// \returns the physical device features (for e.g. checking if a feature is available)
		const vk::PhysicalDeviceFeatures &features() const noexcept;
		/// \returns the available memory (type) properties for the physical device
		const vk::PhysicalDeviceMemoryProperties &memory_properties() const noexcept;

		/// \returns the command_pool that commands can be enqueued on.
		const vk::CommandPool &command_pool() const noexcept;
		const vk::CommandPool &transfer_command_pool() const noexcept;
		/// \returns the descriptor pool for allocating various pipeline descriptors.
		const vk::DescriptorPool &descriptor_pool() const noexcept;

		/// \returns the device queue for enqueuing commands.
		const vk::Queue &queue() const noexcept;
		const vk::Queue &transfer_queue() const noexcept;

		/// \returns the queue index the graphics pipeline is running on.
		uint32_t graphics_queue_index() const noexcept;
		uint32_t transfer_que_index() const noexcept;

		/// \param[in] typeBits the MemoryRequirements you get back from vulkan query operations that resquest the
		/// requirements for resources (like textures and buffers). \param[in] properties the MemoryPropertyFlags of the
		/// requested memory type (i.e. should it be device local, or host, etc...). \param[in, out] typeIndex the
		/// location to store the memory_type index in (on success). \returns if the current context supports a memory
		/// type with the specific properties.
		vk::Bool32 memory_type(uint32_t typeBits, const vk::MemoryPropertyFlags &properties, uint32_t *typeIndex) const
			noexcept;

		/// \param[in] typeBits the MemoryRequirements you get back from vulkan query operations that resquest the
		/// requirements for resources (like textures and buffers).
		/// \param[in] properties the MemoryPropertyFlags of the
		/// requested memory type (i.e. should it be device local, or host, etc...).
		/// \returns a memory type index that supports the given properties.
		uint32_t memory_type(uint32_t typeBits, const vk::MemoryPropertyFlags &properties) const;

		/// \brief flushes all operations on the given commandbuffer and potentially free it at the end.
		/// \param[in] commandBuffer the command buffer to execute the operations on.
		/// \param[in] free defines if we should free the command buffer at the end (true) or not (false).
		void flush(vk::CommandBuffer commandBuffer, bool free);

		// todo we're not using these.
		bool consume(vk::MemoryHeapFlagBits type, vk::DeviceSize amount);
		bool release(vk::MemoryHeapFlagBits type, vk::DeviceSize amount);
		vk::DeviceSize remaining(vk::MemoryHeapFlagBits type);

		/// \returns the device index of the current physical graphics device.
		uint32_t device_index() const noexcept { return m_DeviceIndex; };

		/// \returns true in case the given extension is supported.
		/// \param[in] out_version optionally returns the revision version number.
		bool acquireNextImage2KHR(optional_ref<uint64_t> out_version = std::nullopt) const noexcept;

	  private:
		void init_device();
		void init_debug();
		void init_command_pool();
		void init_descriptor_pool();

		void deinit_device();
		void deinit_debug();
		void deinit_command_pool();
		void deinit_descriptor_pool();

		vk::Result create_device(vk::DeviceQueueCreateInfo *requestedQueues, uint32_t queueSize, vk::Device &device);
		// from a list of all devices, select the one that is the most appropriate (usually the one with the best
		// capabilities)
		void select_physical_device(const std::vector<vk::PhysicalDevice> &allDevices);

		bool queue_index(vk::QueueFlags flags, vk::Queue &queue, uint32_t &queueIndex);

		const bool m_Validated = VULKAN_ENABLE_VALIDATION;
		std::vector<const char *> m_InstanceLayerList;
		std::vector<const char *> m_InstanceExtensionList;

		std::vector<const char *> m_DeviceLayerList;
		std::vector<const char *> m_DeviceExtensionList;

		vk::Instance m_Instance;

		vk::Device m_Device;

		vk::PhysicalDevice m_PhysicalDevice;

		// Stores physical device properties (for e.g. checking device limits)
		vk::PhysicalDeviceProperties m_PhysicalDeviceProperties;
		// Stores phyiscal device features (for e.g. checking if a feature is available)
		vk::PhysicalDeviceFeatures m_PhysicalDeviceFeatures;
		// Stores all available memory (type) properties for the physical device
		vk::PhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;

		vk::CommandPool m_CommandPool;
		vk::CommandPool m_TransferCommandPool;

		vk::DescriptorPool m_DescriptorPool;

		uint32_t m_GraphicsQueueIndex = 0;
		uint32_t m_TransferQueueIndex = 0;

		vk::Queue m_Queue		  = nullptr;
		vk::Queue m_TransferQueue = nullptr;

		VkDebugReportCallbackEXT m_DebugReport;
		// VkDebugReportCallbackCreateInfoEXT debugCBCI;


		struct Memory
		{
			vk::DeviceSize maxMemory;
			vk::DeviceSize availableMemory;
		};

		Memory m_DeviceMemory;
		Memory m_HostMemory;
		uint32_t m_DeviceIndex;
	};
} // namespace core::gfx
