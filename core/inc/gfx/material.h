#pragma once
#include "IDGenerator.h"
#include <optional>
#include "systems/resource.h"
#include "vulkan_stdafx.h"
#include "gfx/details/instance.h"

namespace core::data
{
	class material;
}
namespace core::ivk
{
	class context;
	class shader;
	class geometry;
}

namespace core::gfx
{
	class pipeline;
	class pipeline_cache;
	class texture;
	class sampler;
	class buffer;
	class framebuffer;
	class swapchain;

	/// \brief class that creates a bindable collection of resources that can be used in conjuction with a surface to
	/// render.
	///
	/// The material class is a container of various resources that can, together, describe what should
	/// happen to a surface in the render pipeline on the GPU, and what is all needed.
	/// The material class also can contain instance data and will manage this for you.
	/// Together with a core::ivk::geometry, this describes all the resources you need to render something on screen.
	class material final
	{
		template <typename T, bool use_custom_uid = false>
		using dependency = core::resource::dependency<T, use_custom_uid>;

		template <typename... Ts>
		using packet = core::resource::packet<Ts...>;

	  public:
		using resource_dependency = packet<dependency<core::data::material, true>, psl::UID, core::resource::cache>;

		/// \brief the constructor that will create and bind the necesary resources to create a valid pipeline.
		/// \param[in] packet resource packet containing the data that is needed from the resource system.
		/// \param[in] context a handle to a graphics context (needs to be valid and loaded) which will own the
		/// material.
		/// \param[in] data the material data this instance will be based on.
		/// \param[in] pipeline_cache the pipeline_cache this instance can request a pipeline from.
		/// \param[in] materialBuffer a GPU buffer that can be used by this instance to upload data to (if needed).
		/// \param[in] instanceBuffer a GPU buffer that can be used to upload instance data to, if there is any.
		material(resource_dependency packet, core::resource::handle<core::ivk::context> context,
				 core::resource::handle<core::data::material> data,
				 core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
				 core::resource::handle<core::gfx::buffer> materialBuffer);
		material() = delete;
		~material();
		material(const material&) = delete;
		material(material&&)	  = delete;
		material& operator=(const material&) = delete;
		material& operator=(material&&) = delete;

		/// \brief returns a handle to the material data used to construct this object.
		/// \note when editing the material or the data after construction, this value will be out of sync with the
		/// runtime gfx::material.
		core::resource::handle<core::data::material> data() const;
		/// \brief returns all the shaders that are being used right now by this material.
		const std::vector<core::resource::handle<core::ivk::shader>>& shaders() const;
		/// \brief returns all currently used textures and their binding slots.
		const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::texture>>>& textures() const;
		/// \brief returns all currently used samplers and their binding slots.
		const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::sampler>>>& samplers() const;
		/// \brief returns all currently used buffers and their binding slots.
		/// \note the buffers could be anything, they could be uniform buffer objects, or maybe shader storage buffer
		/// objects.
		const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::buffer>>>& buffers() const;

		/// \brief prepares the material for rendering by binding the pipeline.
		/// \warning only call this in the context of recording the draw call.
		/// \param[in] cmdBuffer the command buffer you'll be recording to
		/// \param[in] framebuffer the framebuffer the pipeline will be bound to.
		/// \param[in] drawIndex the index to be set in the push constant.
		/// \todo drawindex is a temporary hack to support instancing. a generic solution should be sought after.
		bool bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<framebuffer> framebuffer,
						   uint32_t drawIndex);

		/// \brief prepares the material for rendering by binding the pipeline.
		/// \warning only call this in the context of recording the draw call.
		/// \param[in] cmdBuffer the command buffer you'll be recording to
		/// \param[in] swapchain the swapchain the pipeline will be bound to.
		/// \param[in] drawIndex the index to be set in the push constant.
		bool bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<swapchain> swapchain,
						   uint32_t drawIndex);

	  private:
		/// \returns the pipeline this material instance uses for the given framebuffer.
		/// \details tries to find, and return a core::gfx::pipeline that can satisfy the
		/// requirements of this material. In case none is present, then one will be created instead.
		/// \param[in] framebuffer the framebuffer to bind to.
		core::resource::handle<pipeline> get(core::resource::handle<framebuffer> framebuffer);

		/// \returns the pipeline this material instance uses for the given framebuffer.
		/// \details tries to find, and return a core::gfx::pipeline that can satisfy the
		/// requirements of this material. In case none is present, then one will be created instead.
		/// \param[in] swapchain the swapchain to bind to.
		core::resource::handle<pipeline> get(core::resource::handle<swapchain> swapchain);

		psl::UID m_UID;
		core::resource::handle<core::ivk::context> m_Context;
		core::resource::handle<core::gfx::pipeline_cache> m_PipelineCache;
		core::resource::handle<core::data::material> m_Data;

		std::vector<core::resource::handle<core::ivk::shader>> m_Shaders;

		// a combination of binding slot + resource
		std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::texture>>> m_Textures;
		std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::sampler>>> m_Samplers;
		std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::buffer>>> m_Buffers;

		core::resource::handle<core::gfx::buffer> m_MaterialBuffer;
		memory::segment m_MaterialData;

		// psl::UID maps to the psl::UID of a framebuffer or a swapchain
		std::unordered_map<psl::UID, core::resource::handle<core::gfx::pipeline>> m_Pipeline;
		core::resource::handle<core::gfx::pipeline> m_Bound;

		// value to indicate if this material can actually be used or not
		bool m_IsValid{true};
	};
} // namespace core::gfx
