#pragma once

namespace core::data
{
	class material;
}
namespace core::gfx
{
	class context;

	/// \brief encapsulated the concept of a graphics pipeline on the GPU
	class pipeline
	{
	public:
		pipeline(const UID& uid, 
				 core::resource::cache& cache, 
				 core::resource::handle<context> context, 
				 core::resource::handle<core::data::material> data,
				 vk::PipelineCache& pipelineCache,
				 vk::RenderPass renderPass,
				 uint32_t attachmentCount);
		~pipeline();
		pipeline(const pipeline&) = delete;
		pipeline(pipeline&&) = delete;
		pipeline& operator=(const pipeline&) = delete;
		pipeline& operator=(pipeline&&) = delete;

		/// \returns if the pipeline uses any push constants.
		bool has_pushconstants() const noexcept;
		/// \returns the vulkan pipeline object of this instance.
		vk::Pipeline vkPipeline() const noexcept { return m_Pipeline; };
		/// \returns the vulkan pipeline layout of this instance.
		vk::PipelineLayout vkLayout() const noexcept { return m_PipelineLayout; };
		/// \returns the allocated descriptor set for this instance.
		vk::DescriptorSet const * vkDescriptorSet() const noexcept { return &m_DescriptorSet; }

		/// \returns true if there was a binding at that binding location.
		/// \param[in] bindingLocation the binding location to check.
		/// \param[out] out the resulting descriptor set.
		bool get(uint32_t bindingLocation, vk::WriteDescriptorSet &out);

		/// \returns if updating that binding location was successful. It has to be a binding point that exists, and is of the correct type, otherwise it returns false.
		/// \param[in] bindingLocation the binding location to update.
		/// \param[in] descriptor the new information to emplace at the location.
		bool update(uint32_t bindingLocation, vk::WriteDescriptorSet descriptor);

		/// \returns if updating that binding location was successful. It has to be a eCombinedImageSampler descriptor binding point to be successful.
		/// \param[in] bindingLocation the binding location to update.
		/// \param[in] textureMeta the new texture to bind to that location.
		/// \param[in] samplerMeta the new sampler to bind to that location.
		bool update(uint32_t bindingLocation, const UID& textureMeta, const UID& samplerMeta);
		/// \returns if updating that binding location was successful. It has to be an eStorageBuffer or eUniformBuffer descriptor binding point to be successful. 
		/// \param[in] bindingLocation the binding location to update. 
		/// \param[in] offset the new offset of the buffer binding.
		/// \param[in] range the new size of the buffer binding.
		bool update(uint32_t bindingLocation, vk::DeviceSize offset, vk::DeviceSize range);
	private:
		bool update(core::resource::cache& cache, const core::data::material& data, vk::DescriptorSet set);
		core::resource::handle<context> m_Context;

		vk::DescriptorSet m_DescriptorSet;
		vk::DescriptorSetLayout m_DescriptorSetLayout;
		vk::PipelineLayout m_PipelineLayout;
		vk::Pipeline m_Pipeline;
		vk::PipelineCache& m_PipelineCache;
		
		std::vector<vk::WriteDescriptorSet> m_DescriptorSets;
		core::resource::cache& m_Cache;
		bool m_HasPushConstants{false};
		bool m_IsValid{true};
	};
}
