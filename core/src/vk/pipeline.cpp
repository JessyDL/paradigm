
#include "core/vk/pipeline.hpp"
#include "core/data/buffer.hpp"
#include "core/data/material.hpp"
#include "core/meta/shader.hpp"
#include "core/meta/texture.hpp"
#include "core/vk/buffer.hpp"
#include "core/vk/context.hpp"
#include "core/vk/conversion.hpp"
#include "core/vk/sampler.hpp"
#include "core/vk/shader.hpp"
#include "core/vk/texture.hpp"

#include "core/gfx/buffer.hpp"

using namespace psl;
using namespace core::gfx;
using namespace core::ivk;
using namespace core::resource;

inline size_t get_aligned(const core::ivk::context& context, vk::DescriptorType flags, size_t value) {
	auto alignment = (vk::DescriptorType::eUniformBuffer == flags || vk::DescriptorType::eUniformBufferDynamic == flags)
					   ? context.limits().uniform.alignment
					   : context.limits().storage.alignment;
	auto remainder = value % alignment;
	return (remainder) ? value + (alignment - remainder) : value;
}

bool decode(core::resource::cache_t& cache,
			const core::data::material_t& data,
			std::vector<vk::DescriptorSetLayoutBinding>& layoutBinding) {
	for(auto& stage : data.stages()) {
		auto shader_handle = cache.library().get<core::meta::shader>(stage.shader()).value_or(nullptr);
		if(!shader_handle) {
			LOG_ERROR("tried to load incorrect shader ", utility::to_string(stage.shader()));
			return false;
		}

		for(auto& binding : stage.bindings()) {
			switch(binding.descriptor()) {
			case core::gfx::binding_type::combined_image_sampler: {
				vk::DescriptorSetLayoutBinding setLayoutBinding;
				setLayoutBinding.descriptorType = conversion::to_vk(binding.descriptor());
				setLayoutBinding.stageFlags		= conversion::to_vk(shader_handle->stage());
				setLayoutBinding.binding		= binding.binding_slot();
				// Default value in all examples
				setLayoutBinding.descriptorCount	= 1;
				setLayoutBinding.pImmutableSamplers = nullptr;

				layoutBinding.push_back(setLayoutBinding);
			} break;
			case core::gfx::binding_type::uniform_buffer_dynamic:
			case core::gfx::binding_type::storage_buffer_dynamic:
			case core::gfx::binding_type::storage_buffer:
			case core::gfx::binding_type::uniform_buffer: {
				layoutBinding.push_back(
				  utility::vulkan::defaults::descriptor_setlayout_binding(conversion::to_vk(binding.descriptor()),
																		  conversion::to_vk(shader_handle->stage()),
																		  binding.binding_slot()));
			} break;
			default:
				throw new std::runtime_error("this should not be reached");
			}
		}
	}
	return true;
}

bool decode(core::resource::cache_t& cache,
			const core::data::material_t& data,
			std::vector<vk::VertexInputBindingDescription>& vertexBindingDescriptions,
			std::vector<vk::VertexInputAttributeDescription>& vertexAttributeDescriptions) {
	for(auto& stage : data.stages()) {
		auto shader_handle = cache.library().get<core::meta::shader>(stage.shader()).value_or(nullptr);
		if(!shader_handle) {
			LOG_ERROR("tried to load incorrect shader ", utility::to_string(stage.shader()));
			return false;
		}

		if(stage.shader_stage() != core::gfx::shader_stage::vertex &&
		   stage.shader_stage() != core::gfx::shader_stage::compute)
			continue;

		for(auto& attribute : shader_handle->inputs()) {
			vk::VertexInputBindingDescription& vkVertexBinding = vertexBindingDescriptions.emplace_back();
			vkVertexBinding.binding							   = attribute.location();
			vkVertexBinding.stride							   = attribute.size();

			auto input_rate = std::find_if(std::begin(stage.attributes()),
										   std::end(stage.attributes()),
										   [location = attribute.location()](const auto& attribute) noexcept {
											   return location == attribute.location();
										   });

			vkVertexBinding.inputRate =
			  conversion::to_vk(input_rate->input_rate().value_or(core::gfx::vertex_input_rate::vertex));

			for(uint32_t i = 0; i < static_cast<uint32_t>(attribute.count()); ++i) {
				vk::VertexInputAttributeDescription& vkVertexAttribute = vertexAttributeDescriptions.emplace_back();
				vkVertexAttribute.binding							   = vkVertexBinding.binding;
				vkVertexAttribute.location							   = vkVertexBinding.binding + i;
				vkVertexAttribute.format							   = conversion::to_vk(attribute.format());
				vkVertexAttribute.offset							   = attribute.stride() * i;
			}
		}
	}

	return true;
}


pipeline::pipeline(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   core::resource::handle<core::ivk::context> context,
				   core::resource::handle<core::data::material_t> data,
				   vk::PipelineCache& pipelineCache,
				   vk::RenderPass renderPass,
				   uint32_t attachmentCount)
	: m_Context(context), m_PipelineCache(pipelineCache), m_Cache(cache) {
	std::vector<vk::DescriptorSetLayoutBinding> layoutBinding;
	if(!decode(cache, data.value(), layoutBinding)) {
		core::ivk::log->error("fatal error happened during the creation of pipeline {}", metaData.uid.to_string());
		m_IsValid = false;
		return;
	}

	vk::DescriptorSetLayoutCreateInfo descriptorLayout;
	descriptorLayout.pNext		  = nullptr;
	descriptorLayout.bindingCount = (uint32_t)layoutBinding.size();
	descriptorLayout.pBindings	  = layoutBinding.data();

	utility::vulkan::check(
	  m_Context->device().createDescriptorSetLayout(&descriptorLayout, nullptr, &m_DescriptorSetLayout));


	vk::PipelineVertexInputStateCreateInfo VertexInputState;
	std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions;
	std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
	if(!decode(cache, data.value(), vertexBindingDescriptions, vertexAttributeDescriptions)) {
		core::ivk::log->error("fatal error happened during the creation of pipeline {}", metaData.uid.to_string());
		m_IsValid = false;
		return;
	}

	// Assign to vertex input state
	VertexInputState.flags							 = vk::PipelineVertexInputStateCreateFlags();
	VertexInputState.vertexBindingDescriptionCount	 = (uint32_t)vertexBindingDescriptions.size();
	VertexInputState.pVertexBindingDescriptions		 = vertexBindingDescriptions.data();
	VertexInputState.vertexAttributeDescriptionCount = (uint32_t)vertexAttributeDescriptions.size();
	VertexInputState.pVertexAttributeDescriptions	 = vertexAttributeDescriptions.data();

	vk::PipelineLayoutCreateInfo pPipelineLayoutCreateInfo;
	pPipelineLayoutCreateInfo.pNext			 = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts	 = &m_DescriptorSetLayout;

	psl::string_view global = "GLOBAL_";

	// todo: this should be removed in favour of a generic check for push_constants in the shader meta
	for(const auto& stage : data.value().stages()) {
		if(stage.shader_stage() == core::gfx::shader_stage::compute)
			m_BindPoint = vk::PipelineBindPoint::eCompute;

		for(auto& binding : stage.bindings()) {
			if(binding.descriptor() == core::gfx::binding_type::uniform_buffer) {
				// todo: this is a hardcoded setup, brittle and needs to be removed
				/*if(cache.library().has_tag(binding.buffer(), "GLOBAL_WORLD_VIEW_PROJECTION_MATRIX"))
				{
					vk::PushConstantRange pushConstantRange;
					pushConstantRange.offset = 0;
					pushConstantRange.setSize(sizeof(uint32_t));
					pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eVertex);

					pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
					pPipelineLayoutCreateInfo.pPushConstantRanges	 = &pushConstantRange;
					m_HasPushConstants								 = true;
					break;
				}*/
			}
		}
	}


	if(!utility::vulkan::check(
		 m_Context->device().createPipelineLayout(&pPipelineLayoutCreateInfo, nullptr, &m_PipelineLayout))) {
		core::ivk::log->error("fatal error happened during the creation of a pipeline");
		m_IsValid = false;
		return;
	}


	// Load shaders
	// Shaders are loaded from the SPIR-V format, which can be generated from glsl
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	for(auto& stage : data->stages()) {
		auto shader_handle = cache.find<core::ivk::shader>(stage.shader());
		if((shader_handle.state() == core::resource::status::loaded) && shader_handle->pipeline()) {
			shaderStages.push_back(shader_handle->pipeline().value());
		} else {
			core::ivk::log->error("could not load the shader used in the creation of a pipeline");
			m_IsValid = false;
			return;
		}
	}

	core::ivk::log->info("creating a pipeline layout with a {} bindpoint",
						 ((m_BindPoint == vk::PipelineBindPoint::eCompute) ? "compute" : "graphics"));

	if(m_BindPoint == vk::PipelineBindPoint::eCompute) {
		psl_assert(shaderStages.size() == 1,
				   "expecting only a single compute in the shader stages, but got {}",
				   shaderStages.size());

		vk::ComputePipelineCreateInfo pipelineCreateInfo {};
		pipelineCreateInfo.layout = m_PipelineLayout;
		pipelineCreateInfo.stage  = shaderStages[0];

		if(auto pipeline = m_Context->device().createComputePipeline(m_PipelineCache, pipelineCreateInfo);
		   utility::vulkan::check(pipeline)) {
			m_Pipeline = pipeline.value;
		} else {
			core::ivk::log->error("failed to create a compute pipeline");
			m_IsValid = false;
			return;
		}
	} else {
		vk::GraphicsPipelineCreateInfo pipelineCreateInfo {};
		// The layout used for this pipeline
		pipelineCreateInfo.layout = m_PipelineLayout;
		// Renderpass this pipeline is attached to
		pipelineCreateInfo.renderPass = renderPass;

		pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
		pipelineCreateInfo.pStages	  = shaderStages.data();

		// Vertex input state
		// Describes the topoloy used with this pipeline
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
		// This pipeline renders vertex data as triangle lists
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;

		// Rasterization state
		vk::PipelineRasterizationStateCreateInfo rasterizationState;
		rasterizationState.polygonMode		= (data->wireframe()) ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
		rasterizationState.cullMode			= conversion::to_vk(data->cull_mode());
		rasterizationState.frontFace		= vk::FrontFace::eCounterClockwise;	   // default winding
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.depthBiasEnable		   = VK_FALSE;
		rasterizationState.lineWidth			   = 1.0f;

		// Color blend state
		// Describes blend modes and color masks
		vk::PipelineColorBlendStateCreateInfo colorBlendState;

		// One blend attachment state
		// Blending is not used in this example
		std::vector<vk::PipelineColorBlendAttachmentState> blendAttachmentState;
		blendAttachmentState.resize(attachmentCount);

		const auto& blendState = data->blend_states();

		for(size_t i = 0; i < std::min(blendState.size(), (size_t)attachmentCount); ++i) {
			blendAttachmentState[i].blendEnable	   = blendState[i].enabled();
			blendAttachmentState[i].colorWriteMask = conversion::to_vk(blendState[i].color_components());
			if(blendAttachmentState[i].blendEnable) {
				blendAttachmentState[i].srcColorBlendFactor = conversion::to_vk(blendState[i].color_blend_src());
				blendAttachmentState[i].dstColorBlendFactor = conversion::to_vk(blendState[i].color_blend_dst());
				blendAttachmentState[i].colorBlendOp		= conversion::to_vk(blendState[i].color_blend_op());

				blendAttachmentState[i].srcAlphaBlendFactor = conversion::to_vk(blendState[i].alpha_blend_src());
				blendAttachmentState[i].dstAlphaBlendFactor = conversion::to_vk(blendState[i].alpha_blend_dst());
				blendAttachmentState[i].alphaBlendOp		= conversion::to_vk(blendState[i].alpha_blend_op());
			}
		}

		// fill in the remaining with the default blend state;
		core::data::material_t::blendstate def_state;
		for(size_t i = blendState.size(); i < attachmentCount; ++i) {
			blendAttachmentState[i].blendEnable	   = def_state.enabled();
			blendAttachmentState[i].colorWriteMask = conversion::to_vk(def_state.color_components());
			if(blendAttachmentState[i].blendEnable) {
				blendAttachmentState[i].srcColorBlendFactor = conversion::to_vk(def_state.color_blend_src());
				blendAttachmentState[i].dstColorBlendFactor = conversion::to_vk(def_state.color_blend_dst());
				blendAttachmentState[i].colorBlendOp		= conversion::to_vk(def_state.color_blend_op());

				blendAttachmentState[i].srcAlphaBlendFactor = conversion::to_vk(def_state.alpha_blend_src());
				blendAttachmentState[i].dstAlphaBlendFactor = conversion::to_vk(def_state.alpha_blend_dst());
				blendAttachmentState[i].alphaBlendOp		= conversion::to_vk(def_state.alpha_blend_op());
			}
		}
		colorBlendState.attachmentCount = attachmentCount;
		colorBlendState.pAttachments	= blendAttachmentState.data();

		// Viewport state
		vk::PipelineViewportStateCreateInfo viewportState;
		// One viewport
		viewportState.viewportCount = 1;
		// One scissor rectangle
		viewportState.scissorCount = 1;

		// Enable dynamic states
		// Describes the dynamic states to be used with this pipeline
		// Dynamic states can be set even after the pipeline has been created
		// So there is no need to create new pipelines just for changing
		// a viewport's dimensions or a scissor box
		// The dynamic state properties themselves are stored in the command buffer
		std::vector<vk::DynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(vk::DynamicState::eViewport);
		dynamicStateEnables.push_back(vk::DynamicState::eScissor);
		dynamicStateEnables.push_back(vk::DynamicState::eDepthBias);

		vk::PipelineDynamicStateCreateInfo dynamicState;
		dynamicState.pDynamicStates	   = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

		// Depth and stencil state
		// Describes depth and stenctil test and compare ops
		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		// Basic depth compare setup with depth writes and depth test enabled
		// No stencil used
		depthStencilState.depthTestEnable		= data->depth_test();
		depthStencilState.depthWriteEnable		= data->depth_write();
		depthStencilState.depthCompareOp		= vk::CompareOp::eLessOrEqual;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.back.failOp			= vk::StencilOp::eKeep;
		depthStencilState.back.passOp			= vk::StencilOp::eKeep;
		depthStencilState.back.compareOp		= vk::CompareOp::eAlways;
		depthStencilState.stencilTestEnable		= VK_FALSE;
		depthStencilState.front					= depthStencilState.back;

		// Multi sampling state
		vk::PipelineMultisampleStateCreateInfo multisampleState;
		multisampleState.pSampleMask = NULL;
		// todo: deal with multi sampling
		multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;


		// Assign states
		// Assign pipeline state create information
		pipelineCreateInfo.pVertexInputState   = &VertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState	   = &colorBlendState;
		pipelineCreateInfo.pMultisampleState   = &multisampleState;
		pipelineCreateInfo.pViewportState	   = &viewportState;
		pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
		pipelineCreateInfo.renderPass		   = renderPass;
		pipelineCreateInfo.pDynamicState	   = &dynamicState;

		// Create rendering pipeline
		if(!utility::vulkan::check(m_Context->device().createGraphicsPipelines(
			 m_PipelineCache, 1, &pipelineCreateInfo, nullptr, &m_Pipeline))) {
			debug_break();
		}
	}
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts		 = &m_DescriptorSetLayout;
	allocInfo.descriptorPool	 = m_Context->descriptor_pool();

	utility::vulkan::check(m_Context->device().allocateDescriptorSets(&allocInfo, &m_DescriptorSet));


	update(m_Cache, data.value(), m_DescriptorSet);
}

pipeline::~pipeline() {
	m_Context->device().destroyPipeline(m_Pipeline, nullptr);
	m_Context->device().destroyPipelineLayout(m_PipelineLayout, nullptr);
	m_Context->device().destroyDescriptorSetLayout(m_DescriptorSetLayout, nullptr);
}

inline size_t get_range(const core::ivk::context& context,
						const core::meta::shader& shader,
						const core::data::material_t::binding& binding,
						size_t fallback) {
	auto it =
	  std::find_if(std::begin(shader.descriptors()),
				   std::end(shader.descriptors()),
				   [&binding](const auto& descriptor) { return descriptor.binding() == binding.binding_slot(); });
	if(it != std::end(shader.descriptors()))
		fallback = it->size();

	return get_aligned(context, conversion::to_vk(binding.descriptor()), fallback);
}

bool pipeline::update(core::resource::cache_t& cache, const core::data::material_t& data, vk::DescriptorSet set) {
	m_IsComplete = true;
	for(const auto& stage : data.stages()) {
		auto shader_handle = cache.find<core::ivk::shader>(stage.shader());
		if((shader_handle.state() == core::resource::status::loaded) && shader_handle->pipeline()) {
			for(const auto& binding : stage.bindings()) {
				switch(binding.descriptor()) {
				case core::gfx::binding_type::combined_image_sampler: {
					auto tex_handle = cache.find<core::ivk::texture_t>(binding.texture());
					if(!binding.texture()) {
						core::ivk::log->error(
						  "Fatal error, tried to load a non-existing UID into the pipeline related to the material");
						m_IsValid = false;
						return false;
					}

					if(tex_handle.state() != core::resource::status::loaded) {
						tex_handle = cache.create_using<core::ivk::texture_t>(binding.texture(), m_Context);

						if(tex_handle.state() != core::resource::status::loaded) {
							LOG_ERROR("could not load the texture ",
									  utility::to_string(binding.texture()),
									  " when updating the pipeline");

							m_IsValid = false;
							return false;
						}
					}

					vk::WriteDescriptorSet writeDescriptorSet {};
					writeDescriptorSet.pNext		   = nullptr;
					writeDescriptorSet.dstSet		   = set;
					writeDescriptorSet.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
					writeDescriptorSet.dstBinding	   = binding.binding_slot();
					writeDescriptorSet.pImageInfo	   = &tex_handle->descriptor(binding.sampler());
					writeDescriptorSet.descriptorCount = 1;
					writeDescriptorSet.pBufferInfo	   = nullptr;

					m_DescriptorSets.push_back(writeDescriptorSet);
				} break;
				case core::gfx::binding_type::storage_buffer:
				case core::gfx::binding_type::uniform_buffer:
				case core::gfx::binding_type::storage_buffer_dynamic:
				case core::gfx::binding_type::uniform_buffer_dynamic: {
					auto& writeDescriptorSet		  = m_DescriptorSets.emplace_back();
					writeDescriptorSet.pNext		  = NULL;
					writeDescriptorSet.dstSet		  = set;
					writeDescriptorSet.descriptorType = conversion::to_vk(binding.descriptor());
					writeDescriptorSet.dstBinding	  = binding.binding_slot();
					writeDescriptorSet.pImageInfo	  = nullptr;
					writeDescriptorSet.pBufferInfo =
					  m_TrackedBufferInfos.emplace_back(std::make_unique<vk::DescriptorBufferInfo>()).get();

					// todo: investigate using this
					writeDescriptorSet.descriptorCount = 1;

					auto buffer_handle = cache.find<core::gfx::shader_buffer_binding>(binding.buffer());
					if(!buffer_handle) {
						core::ivk::log->warn("Tried to use the missing buffer {} in a pipeline in shader {}",
											 vk::to_string(conversion::to_vk(binding.descriptor())),
											 utility::to_string(stage.shader()));
						core::ivk::log->warn("Loading will be deferred");
						m_IsComplete = false;
					} else if(buffer_handle.state() == core::resource::status::loaded) {
						vk::BufferUsageFlagBits usage =
						  (binding.descriptor() == core::gfx::binding_type::uniform_buffer ||
						   binding.descriptor() == core::gfx::binding_type::uniform_buffer_dynamic)
							? vk::BufferUsageFlagBits::eUniformBuffer
							: vk::BufferUsageFlagBits::eStorageBuffer;
						if(!(conversion::to_vk(buffer_handle->buffer->data().usage()) & usage)) {
							LOG_ERROR("The actual buffer's usage is not compatible with the buffer type ",
									  vk::to_string(usage));
							LOG_ERROR("Shader: ", utility::to_string(stage.shader()));
							m_IsComplete = false;
						}

						auto& bufferInfo = m_TrackedBufferInfos[m_TrackedBufferInfos.size() - 1];
						bufferInfo->buffer =
						  buffer_handle->buffer->resource<gfx::graphics_backend::vulkan>()->gpu_buffer();
						bufferInfo->offset = 0;
						// todo this lookup can be improved
						bufferInfo->range = get_range(
						  m_Context.value(), *shader_handle.meta(), binding, buffer_handle->buffer->data().size());
					} else {
						core::ivk::log->warn("Tried to use the unloaded {} in a pipeline in shader {}",
											 vk::to_string(conversion::to_vk(binding.descriptor())),
											 utility::to_string(stage.shader()));
						m_IsComplete = false;
					}

					// TODO: Cache the buffers needed and add references to them.
				} break;
				default:
					m_IsValid = false;
					throw new std::runtime_error("This should not be reached");
					return false;
				}
			}
		} else {
			LOG_ERROR("could not load the shader used in the pipeline when trying to update the descriptor sets");
			return false;
		}
	}

	if(m_IsComplete)
		m_Context->device().updateDescriptorSets(
		  static_cast<uint32_t>(m_DescriptorSets.size()), m_DescriptorSets.data(), 0, nullptr);
	return m_IsComplete;
}

bool pipeline::update(uint32_t bindingLocation, vk::WriteDescriptorSet descriptor) {
	for(auto& set : m_DescriptorSets) {
		if(set.dstBinding == bindingLocation) {
			if(set.descriptorType != descriptor.descriptorType) {
				LOG_ERROR("Tried to set a DescriptorSet of the wrong type");
				return false;
			}
			set		   = descriptor;
			set.dstSet = m_DescriptorSet;
			m_Context->device().updateDescriptorSets(
			  static_cast<uint32_t>(m_DescriptorSets.size()), m_DescriptorSets.data(), 0, nullptr);
			return true;
		}
	}

	LOG_ERROR("Could not find the binding location in the pipeline");
	return false;
}
bool pipeline::update(uint32_t bindingLocation, const UID& textureMeta, const UID& samplerMeta) {
	for(auto& set : m_DescriptorSets) {
		if(set.dstBinding == bindingLocation) {
			switch(set.descriptorType) {
			case vk::DescriptorType::eCombinedImageSampler: {
				auto tex_handle = m_Cache.find<core::ivk::texture_t>(textureMeta);

				if(tex_handle.state() != core::resource::status::loaded) {
					tex_handle = m_Cache.create_using<core::ivk::texture_t>(textureMeta, m_Context);

					if(tex_handle.state() != core::resource::status::loaded) {
						LOG_ERROR("could not load the texture ",
								  utility::to_string(textureMeta),
								  " when updating the pipeline");

						m_IsValid = false;
						return false;
					}
				}

				set.pImageInfo = &tex_handle->descriptor(samplerMeta);
				m_Context->device().updateDescriptorSets(
				  static_cast<uint32_t>(m_DescriptorSets.size()), m_DescriptorSets.data(), 0, nullptr);
				return true;
			} break;
			default:
				LOG_ERROR("The descriptor type [" + vk::to_string((vk::DescriptorType)set.descriptorType) +
						  "] is not a Combine Image Sampler Type.");
				return false;
			}
		}
	}

	LOG_ERROR("Could not find the binding location in the pipeline");
	return false;
}
bool pipeline::update(uint32_t bindingLocation, vk::DeviceSize offset, vk::DeviceSize range) {
	for(auto& set : m_DescriptorSets) {
		if(set.dstBinding == bindingLocation) {
			switch(set.descriptorType) {
			case vk::DescriptorType::eStorageBuffer:
			case vk::DescriptorType::eUniformBuffer:
			case vk::DescriptorType::eUniformBufferDynamic:
			case vk::DescriptorType::eStorageBufferDynamic: {
				auto bufferInfo = std::make_unique<vk::DescriptorBufferInfo>();
				auto oldbuffer	= set.pBufferInfo;

				bufferInfo->buffer = set.pBufferInfo->buffer;
				bufferInfo->offset = offset;
				bufferInfo->range  = get_aligned(m_Context.value(), set.descriptorType, range);
				set.pBufferInfo	   = bufferInfo.get();

				m_Context->device().updateDescriptorSets(
				  static_cast<uint32_t>(m_DescriptorSets.size()), m_DescriptorSets.data(), 0, nullptr);

				*std::find_if(std::begin(m_TrackedBufferInfos),
							  std::end(m_TrackedBufferInfos),
							  [oldbuffer](const auto& ptr) { return oldbuffer == ptr.get(); }) = std::move(bufferInfo);
				return true;
			} break;
			default:
				LOG_ERROR("The descriptor type [" + vk::to_string((vk::DescriptorType)set.descriptorType) +
						  "] is not a buffer.");
				return false;
			}
		}
	}

	LOG_ERROR("Could not find the binding location in the pipeline");
	return false;
}

bool pipeline::unsafe_update(uint32_t bindingLocation,
							 core::resource::handle<core::ivk::buffer_t> buffer,
							 vk::DeviceSize offset,
							 vk::DeviceSize range) {
	for(auto& set : m_DescriptorSets) {
		if(set.dstBinding == bindingLocation) {
			switch(set.descriptorType) {
			case vk::DescriptorType::eStorageBuffer:
			case vk::DescriptorType::eUniformBuffer:
			case vk::DescriptorType::eUniformBufferDynamic:
			case vk::DescriptorType::eStorageBufferDynamic: {
				auto bufferInfo = std::make_unique<vk::DescriptorBufferInfo>();
				auto oldbuffer	= set.pBufferInfo;

				bufferInfo->buffer = buffer->buffer_info().buffer;
				bufferInfo->offset = offset;
				bufferInfo->range  = get_aligned(m_Context.value(), set.descriptorType, range);
				set.pBufferInfo	   = bufferInfo.get();
				m_Context->device().updateDescriptorSets(
				  static_cast<uint32_t>(m_DescriptorSets.size()), m_DescriptorSets.data(), 0, nullptr);


				*std::find_if(std::begin(m_TrackedBufferInfos),
							  std::end(m_TrackedBufferInfos),
							  [oldbuffer](const auto& ptr) { return oldbuffer == ptr.get(); }) = std::move(bufferInfo);

				if(!m_IsComplete)
					completeness_check();
				return true;
			}
			}
		}
	}

	LOG_ERROR("Could not find the binding location in the pipeline");
	return false;
}
bool pipeline::get(uint32_t bindingLocation, vk::WriteDescriptorSet& out) {
	if(auto it = std::find_if(std::begin(m_DescriptorSets),
							  std::end(m_DescriptorSets),
							  [bindingLocation](const auto& set) { return set.dstBinding == bindingLocation; });
	   it != std::end(m_DescriptorSets)) {
		out = *it;
		return true;
	}

	LOG_ERROR("Could not find the binding location in the pipeline");
	return false;
}

bool pipeline::has_pushconstants() const noexcept {
	return m_HasPushConstants;
}

bool pipeline::completeness_check() noexcept {
	m_IsComplete = true;
	for(const auto& set : m_DescriptorSets) {
		if(set.pBufferInfo == nullptr && set.pImageInfo == nullptr && set.pTexelBufferView == nullptr) {
			m_IsComplete = false;
			return false;
		}
	}
	return true;
}


bool pipeline::bind(vk::CommandBuffer& buffer, psl::array_view<uint32_t> dynamicOffsets) {
	buffer.bindPipeline(m_BindPoint, m_Pipeline);
	buffer.bindDescriptorSets(m_BindPoint,
							  m_PipelineLayout,
							  0,
							  1,
							  &m_DescriptorSet,
							  static_cast<uint32_t>(dynamicOffsets.size()),
							  dynamicOffsets.data());

	return true;
}
