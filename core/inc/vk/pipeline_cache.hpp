#pragma once
#include "resource/resource.hpp"
#include "vk/ivk.hpp"

namespace core::ivk {
class context;
class pipeline;
class framebuffer_t;
class swapchain;
}	 // namespace core::ivk

namespace core::data {
class material_t;
}

namespace psl {
struct UID;
}


namespace core::ivk {
/// \brief the pipeline key creates a hash of the important elements of a vk::Pipeline
///
/// when you want to store, and lookup pipelines based on their properties, then pipeline_key
/// is the way to go. the pipeline key allows you to calculate a hash based on the identifying
/// properties that make it unique (for the GPU), and easily retrieve it.
/// to see this being used, check core::ivk::pipeline_cache.
/// \see core::ivk::pipeline_cache
struct pipeline_key {
	pipeline_key() = default;
	/// \brief constructor based on the data you wish to store.
	/// \warning the pipeline_key does not update when the material data has been updated.
	/// the material will no longer be able to use this key when it changes its properties.
	pipeline_key(const psl::UID& uid, core::resource::handle<core::data::material_t> data, vk::RenderPass pass);

	bool operator==(const pipeline_key& other) const noexcept {
		return renderPass.operator VkRenderPass() == other.renderPass.operator VkRenderPass() &&
			   descriptors.size() == other.descriptors.size() &&
			   std::equal(std::begin(descriptors),
						  std::end(descriptors),
						  std::begin(other.descriptors),
						  std::end(other.descriptors));
	}
	bool operator!=(const pipeline_key& other) const noexcept {
		return renderPass.operator VkRenderPass() != other.renderPass.operator VkRenderPass() ||
			   descriptors.size() != other.descriptors.size() ||
			   !std::equal(std::begin(descriptors),
						   std::end(descriptors),
						   std::begin(other.descriptors),
						   std::end(other.descriptors));
	}

	/// \brief the contained descriptors that describe this key.
	const std::vector<std::pair<vk::DescriptorType, uint32_t>> descriptors;
	/// \brief the bound renderpass of this key.
	const vk::RenderPass renderPass;
	psl::UID uid;
};
}	 // namespace core::ivk

namespace std {
template <>
struct hash<core::ivk::pipeline_key> {
	std::size_t operator()(core::ivk::pipeline_key const& s) const noexcept {
		std::size_t seed = std::hash<psl::UID> {}(s.uid);
		for(auto& i : s.descriptors) {
			seed ^= (uint64_t)i.first + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= (uint64_t)i.second + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		seed += (uint64_t)s.renderPass.operator VkRenderPass() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};
}	 // namespace std

namespace core::ivk {
/// \brief the pipeline cache allows sharing of pipelines between various materials.
///
/// the pipeline cache allows sharing of pipelines between various materials.
/// it is responsible for the creation and destruction of all pipeline objects, as well as
/// providing easy facilities to get pipelines based on material descriptions.
class pipeline_cache {
  public:
	pipeline_cache(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile,
				   core::resource::handle<core::ivk::context> context);
	~pipeline_cache();
	pipeline_cache(const pipeline_cache&)			 = delete;
	pipeline_cache(pipeline_cache&&)				 = delete;
	pipeline_cache& operator=(const pipeline_cache&) = delete;
	pipeline_cache& operator=(pipeline_cache&&)		 = delete;

	/// \brief allows you to get a pipeline that satisfy the material requirements and is bound to the given
	/// framebuffer.
	/// \returns a handle to a pipeline object.
	/// \param[in] data the material containing the description of all bindings.
	/// \param[in] framebuffer the framebuffer that will be bound to.
	core::resource::handle<core::ivk::pipeline> get(const psl::UID& uid,
													core::resource::handle<core::data::material_t> data,
													core::resource::handle<core::ivk::framebuffer_t> framebuffer);
	/// \brief allows you to get a pipeline that satisfy the material requirements and is bound to the given
	/// swapchain.
	/// \returns a handle to a pipeline object.
	/// \param[in] data the material containing the description of all bindings.
	/// \param[in] swapchain the swapchain that will be bound to.
	core::resource::handle<core::ivk::pipeline> get(const psl::UID& uid,
													core::resource::handle<core::data::material_t> data,
													core::resource::handle<core::ivk::swapchain> swapchain);

  private:
	core::resource::handle<core::ivk::context> m_Context;
	core::resource::cache_t& m_Cache;
	vk::PipelineCache m_PipelineCache;

	// std::vector<core::resource::handle<core::ivk::pipeline>> m_Pipelines;

	std::unordered_map<pipeline_key, core::resource::handle<core::ivk::pipeline>> m_Pipelines;
};
}	 // namespace core::ivk
