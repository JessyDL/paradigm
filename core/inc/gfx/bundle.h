#pragma once
#include "array.h"
#include "resource/resource.hpp"
#include "vk/material.h"
#include "gfx/details/instance.h"

namespace core::ivk
{
	class geometry;
}

namespace core::gfx
{
	class buffer;

	class bundle final
	{

	  public:
		bundle(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<core::ivk::buffer> buffer);

		~bundle()				  = default;
		bundle(const bundle&)	 = delete;
		bundle(bundle&&) noexcept = delete;
		bundle& operator=(const bundle&) = delete;
		bundle& operator=(bundle&&) noexcept = delete;
		// ------------------------------------------------------------------------------------------------------------
		// material API
		// ------------------------------------------------------------------------------------------------------------
	  public:
		std::optional<core::resource::handle<core::ivk::material>> get(uint32_t renderlayer) const noexcept;
		bool has(uint32_t renderlayer) const noexcept;

		void set(core::resource::handle<core::ivk::material> material,
				 std::optional<uint32_t> render_layer_override = std::nullopt);


		// ------------------------------------------------------------------------------------------------------------
		// render API
		// ------------------------------------------------------------------------------------------------------------
		psl::array<uint32_t> materialIndices(uint32_t begin, uint32_t end) const noexcept;
		bool bind_material(uint32_t renderlayer) noexcept;

		/// \brief prepares the material for rendering by binding the pipeline.
		/// \warning only call this in the context of recording the draw call.
		/// \param[in] cmdBuffer the command buffer you'll be recording to
		/// \param[in] framebuffer the framebuffer the pipeline will be bound to.
		/// \param[in] drawIndex the index to be set in the push constant.
		/// \todo drawindex is a temporary hack to support instancing. a generic solution should be sought after.
		/// \warning You have to call bind_material before this.
		bool bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<core::ivk::framebuffer> framebuffer,
						   uint32_t drawIndex);

		/// \brief prepares the material for rendering by binding the pipeline.
		/// \warning only call this in the context of recording the draw call.
		/// \param[in] cmdBuffer the command buffer you'll be recording to
		/// \param[in] swapchain the swapchain the pipeline will be bound to.
		/// \param[in] drawIndex the index to be set in the push constant.
		/// \warning You have to call bind_material before this.
		bool bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<core::ivk::swapchain> swapchain,
						   uint32_t drawIndex);

		/// \brief prepares the material for rendering by binding the geometry's instance data.
		/// \warning only call this in the context of recording the draw call, *after* you called bind_pipeline().
		/// \param[in] cmdBuffer the command buffer you'll be recording to
		/// \param[in] geometry the geometry that will be bound.
		/// \warning You have to call bind_pipeline before this.
		bool bind_geometry(vk::CommandBuffer cmdBuffer, const core::resource::handle<core::ivk::geometry> geometry);

		core::resource::handle<core::ivk::material> bound() const noexcept { return m_Bound; };
		// ------------------------------------------------------------------------------------------------------------
		// instance data API
		// ------------------------------------------------------------------------------------------------------------
	  public:
		/// \brief returns the instance count currently used for the given piece of geometry.
		/// \param[in] geometry the geometry to check.
		uint32_t instances(core::resource::tag<core::ivk::geometry> geometry) const noexcept;
		std::vector<std::pair<uint32_t, uint32_t>> instantiate(core::resource::tag<core::ivk::geometry> geometry,
															   uint32_t count = 1);

		uint32_t size(core::resource::tag<core::ivk::geometry> geometry) const noexcept;
		bool has(core::resource::tag<core::ivk::geometry> geometry) const noexcept;
		bool release(core::resource::tag<core::ivk::geometry> geometry, uint32_t id) noexcept;
		bool release_all() noexcept;

		template <typename T>
		bool set(core::resource::tag<core::ivk::geometry> geometry, uint32_t id, psl::string_view name,
				 const psl::array<T>& values)
		{
			static_assert(std::is_trivially_copyable<T>::value, "the type has to be trivially copyable");
			static_assert(std::is_standard_layout<T>::value, "the type has to be is_standard_layout");
			auto res = m_InstanceData.segment(geometry, name);
			if(!res)
			{
				core::gfx::log->error("The element name {} was not found on geometry {}", name, geometry.uid());
				return false;
			}
			return set(geometry, id, res.value().first, res.value().second, values.data(), sizeof(T), values.size());
		}

	  private:
		bool set(core::resource::tag<core::ivk::geometry> geometry, uint32_t id, memory::segment segment,
				 uint32_t size_of_element, const void* data, size_t size, size_t count = 1);

		// ------------------------------------------------------------------------------------------------------------
		// member variables
		// ------------------------------------------------------------------------------------------------------------
	  private:
		details::instance::data m_InstanceData;

		psl::array<core::resource::handle<core::ivk::material>> m_Materials;
		psl::array<uint32_t> m_Layers;
		psl::UID m_UID;
		core::resource::cache& m_Cache;

		core::resource::handle<core::ivk::material> m_Bound;
	};
} // namespace core::gfx