#pragma once

#include "psl/array_view.hpp"
#include "resource/resource.hpp"
#include "vk/ivk.hpp"
#include <vector>

namespace core::data
{
class material_t;
}

namespace core::ivk
{
class context;
class buffer_t;
}	 // namespace core::ivk
namespace core::ivk
{
/// \brief encapsulated the concept of a graphics pipeline on the GPU
/// \warning calling the update method family _during_ the recording of the vk::CommandBuffer's will invalidate the
/// command buffer
/// \todo find a solution around the warning
class pipeline
{
  public:
	/// \brief creates a graphics pipeline
	/// \warning this constructor will error-out when it detects you trying to create a compute pipeline instead
	pipeline(core::resource::cache_t& cache,
			 const core::resource::metadata& metaData,
			 psl::meta::file* metaFile,
			 core::resource::handle<core::ivk::context> context,
			 core::resource::handle<core::data::material_t> data,
			 vk::PipelineCache& pipelineCache,
			 vk::RenderPass renderPass,
			 uint32_t attachmentCount);

	/// \brief creates a compute pipeline
	/// \warning this constructor will error-out when it detects you trying to create a graphics pipeline instead
	pipeline(core::resource::cache_t& cache,
			 const core::resource::metadata& metaData,
			 psl::meta::file* metaFile,
			 core::resource::handle<core::ivk::context> context,
			 core::resource::handle<core::data::material_t> data,
			 vk::PipelineCache& pipelineCache);

	~pipeline();
	pipeline(const pipeline&)			 = delete;
	pipeline(pipeline&&)				 = delete;
	pipeline& operator=(const pipeline&) = delete;
	pipeline& operator=(pipeline&&)		 = delete;

	/// \returns if the pipeline uses any push constants.
	bool has_pushconstants() const noexcept;
	/// \returns the vulkan pipeline object of this instance.
	vk::Pipeline vkPipeline() const noexcept { return m_Pipeline; };
	/// \returns the vulkan pipeline layout of this instance.
	vk::PipelineLayout vkLayout() const noexcept { return m_PipelineLayout; };
	/// \returns the allocated descriptor set for this instance.
	vk::DescriptorSet const* vkDescriptorSet() const noexcept { return &m_DescriptorSet; }

	/// \returns true if there was a binding at that binding location.
	/// \param[in] bindingLocation the binding location to check.
	/// \param[out] out the resulting descriptor set.
	bool get(uint32_t bindingLocation, vk::WriteDescriptorSet& out);

	/// \returns if updating that binding location was successful. It has to be a binding point that exists, and is
	/// of the correct type, otherwise it returns false.
	/// \param[in] bindingLocation the binding location to update.
	/// \param[in] descriptor the new information to emplace at the location.
	bool update(uint32_t bindingLocation, vk::WriteDescriptorSet descriptor);

	/// \returns if updating that binding location was successful. It has to be a eCombinedImageSampler descriptor
	/// binding point to be successful. \param[in] bindingLocation the binding location to update. \param[in]
	/// textureMeta the new texture to bind to that location. \param[in] samplerMeta the new sampler to bind to that
	/// location.
	bool update(uint32_t bindingLocation, const psl::UID& textureMeta, const psl::UID& samplerMeta);

	/// \returns if updating that binding location was successful. It has to be an eStorageBuffer or eUniformBuffer
	/// descriptor binding point to be successful.
	/// \param[in] bindingLocation the binding location to update.
	/// \param[in] offset the new offset of the buffer binding.
	/// \param[in] range the new size of the buffer binding.
	bool update(uint32_t bindingLocation, vk::DeviceSize offset, vk::DeviceSize range);

	/// \returns if updating the binding location was successful.
	/// \warning the buffers's usage flags have to satisfy the requirements of the binding location. If this method
	/// fails inspect the logs to see what specifically went wrong.
	/// \param[in] bindingLocation the binding location to update.
	/// \param[in] buffer the new buffer to bind this descriptorset to.
	/// \param[in] offset the new offset of the buffer binding.
	/// \param[in] range the new size of the buffer binding.
	bool unsafe_update(uint32_t bindingLocation,
					   core::resource::handle<core::ivk::buffer_t> buffer,
					   vk::DeviceSize offset,
					   vk::DeviceSize range);

	/// \returns if the pipeline's descriptors have been completely filled in
	/// \warning complete doesn't mean 'correct'. The descriptors can be filled in to point to missing or deleted items
	inline bool is_complete() const noexcept { return m_IsComplete && is_valid(); }

	/// \returns if the pipeline was successfully created, when false the pipeline is unrecoverable (inspect logs
	/// for reasons)
	inline bool is_valid() const noexcept { return m_IsValid; }

	/// \copydoc is_valid()
	inline operator bool() const noexcept { return is_valid(); }

	bool bind(vk::CommandBuffer& buffer, psl::array_view<uint32_t> dynamicOffsets = {});

  private:
	bool completeness_check() noexcept;
	bool update(core::resource::cache_t& cache, const core::data::material_t& data, vk::DescriptorSet set);
	core::resource::handle<core::ivk::context> m_Context;

	vk::DescriptorSet m_DescriptorSet;
	vk::DescriptorSetLayout m_DescriptorSetLayout;
	vk::PipelineLayout m_PipelineLayout;
	vk::Pipeline m_Pipeline;
	vk::PipelineCache& m_PipelineCache;
	vk::PipelineBindPoint m_BindPoint {vk::PipelineBindPoint::eGraphics};

	std::vector<vk::WriteDescriptorSet> m_DescriptorSets;
	std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> m_TrackedBufferInfos;
	core::resource::cache_t& m_Cache;
	bool m_HasPushConstants {false};
	bool m_IsValid {true};
	bool m_IsComplete {true};
};
}	 // namespace core::ivk
