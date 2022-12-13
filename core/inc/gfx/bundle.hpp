#pragma once
#include "fwd/gfx/buffer.hpp"
#include "fwd/gfx/bundle.hpp"
#include "fwd/gfx/geometry.hpp"
#include "fwd/gfx/material.hpp"
#include "gfx/details/instance.hpp"
#include "psl/array.hpp"
#include "resource/resource.hpp"

namespace core::ivk
{
class geometry_t;
class framebuffer_t;
class swapchain;
class drawpass;
}	 // namespace core::ivk

namespace core::igles
{
class drawpass;
}
namespace vk
{
class CommandBuffer;
}

namespace core::gfx
{
enum class geometry_type
{
	STATIC	= 0,
	DYNAMIC = 1
};
namespace constants
{
	static constexpr psl::string_view INSTANCE_MODELMATRIX		  = "INSTANCE_TRANSFORM";
	static constexpr psl::string_view INSTANCE_LEGACY_MODELMATRIX = "iModelMat";
}	 // namespace constants

/// \detail
/// Bundles acts as a collection of core::gfx::materials, which are associated with a specific renderID (ordered
/// from lowest to highest). These renderID's are what core::gfx::drawpass will use to decide which materials to
/// bind for the current pass when rendering the associated geometry.
///
/// Aside from that, bundles also contain the abstraction of instance data associated with a material-geometry
/// combination. This allows geometry to share instance related data (such as positions, or colors) across different
/// 'core::gfx::material_t's and different 'core::gfx::pass'es. As example sharing instance position data between
/// the depth-only prepass, and normal render pass.
class bundle final
{
	friend class core::ivk::drawpass;
	friend class core::igles::drawpass;

	/*
	todo: Uses a simple allocate front/back mechanism for handling static (front) and dynamic (back) items. This
	helps in mitigating stalls and useless uploads to the GPU.
	*/

  public:
	bundle(core::resource::cache_t& cache,
		   const core::resource::metadata& metaData,
		   psl::meta::file* metaFile,
		   core::resource::handle<core::gfx::buffer_t> vertexBuffer,
		   core::resource::handle<core::gfx::shader_buffer_binding> materialBuffer);

	~bundle()							 = default;
	bundle(const bundle&)				 = delete;
	bundle(bundle&&) noexcept			 = delete;
	bundle& operator=(const bundle&)	 = delete;
	bundle& operator=(bundle&&) noexcept = delete;
	// ------------------------------------------------------------------------------------------------------------
	// material API
	// ------------------------------------------------------------------------------------------------------------
  public:
	std::optional<core::resource::handle<core::gfx::material_t>> get(uint32_t renderlayer) const noexcept;
	bool has(uint32_t renderlayer) const noexcept;

	/// \brief assigns a material to the bundle, with optional render_layer
	/// \details Assigns a material to this bundle. If no render_layer_override is provided, the materials default
	/// value will be used instead.
	void set_material(core::resource::handle<core::gfx::material_t> material,
					  std::optional<uint32_t> render_layer_override = std::nullopt);


	// ------------------------------------------------------------------------------------------------------------
	// render API
	// ------------------------------------------------------------------------------------------------------------
	psl::array<uint32_t> materialIndices(uint32_t begin, uint32_t end) const noexcept;
	bool bind_material(uint32_t renderlayer) noexcept;

	core::resource::handle<core::gfx::material_t> bound() const noexcept { return m_Bound; };
	// ------------------------------------------------------------------------------------------------------------
	// instance data API
	// ------------------------------------------------------------------------------------------------------------
  public:
	/// \brief returns the instance count currently used for the given piece of geometry.
	/// \param[in] geometry UID to check
	uint32_t instances(core::resource::tag<core::gfx::geometry_t> geometry) const noexcept;
	std::vector<std::pair<uint32_t, uint32_t>> instantiate(core::resource::tag<core::gfx::geometry_t> geometry,
														   uint32_t count	  = 1,
														   geometry_type type = geometry_type::STATIC);

	/// \brief returns how many instances are currently active for the given geometry.
	/// \param[in] geometry UID to check
	uint32_t size(core::resource::tag<core::gfx::geometry_t> geometry) const noexcept;

	/// \brief returns if there are *any* active instances for the given geometry.
	/// \param[in] geometry UID to check
	bool has(core::resource::tag<core::gfx::geometry_t> geometry) const noexcept;

	/// \brief release the instance associated with the given ID and geometry
	/// \param[in] geometry target UID
	/// \param[in] id instance ID
	/// \returns true in case the instance was successfully transitioned from active to deactivated.
	bool release(core::resource::tag<core::gfx::geometry_t> geometry, uint32_t id) noexcept;

	/// \brief release all instance data.
	/// \param[in] type optionally target only static or dynamic data
	bool release_all(std::optional<geometry_type> type = {}) noexcept;

	/// \brief set instance data for the given instance (and range)
	/// \param[in] geometry target UID
	/// \param[in] id first instance ID
	/// \param[in] name name of the buffer (present in the shader)
	/// \param[in] values the values to set, where the size + id indicates the end of the range
	/// \returns true if the geometry was found, all instances were present, and the upload dispatched. The upload
	/// is async.
	template <typename T>
	bool set(core::resource::tag<core::gfx::geometry_t> geometry,
			 uint32_t id,
			 psl::string_view name,
			 const psl::array<T>& values)
	{
		static_assert(std::is_trivially_copyable<T>::value, "the type has to be trivially copyable");
		static_assert(std::is_standard_layout<T>::value, "the type has to be is_standard_layout");
		auto res = m_InstanceData.segment(geometry, name);
		if(!res)
		{
			core::gfx::log->error("The element name {} was not found on geometry {}", name, geometry.uid().to_string());
			return false;
		}
		return set(geometry, id, res.value().first, res.value().second, values.data(), sizeof(T), values.size());
	}

	template <typename T>
	bool set(core::resource::tag<core::gfx::material_t> material, const T& value, size_t offset = 0)
	{
		static_assert(std::is_trivially_copyable<T>::value, "the type has to be trivially copyable");
		static_assert(std::is_standard_layout<T>::value, "the type has to be is_standard_layout");
		return set(material, &value, sizeof(T), offset);
	}

	template <typename T>
	bool set(core::resource::tag<core::gfx::material_t> material, psl::string_view name, const T& value)
	{
		auto offset = m_InstanceData.offset_of(material, name);
		if(offset == std::numeric_limits<decltype(offset)>::max())
		{
			core::gfx::log->error(
			  "The element name {} was not found in the material {} data", name, material.uid().to_string());
			return false;
		}
		return set(material, value, offset);
	}

	template <typename T>
	bool set(psl::string_view name, const T& value)
	{
		size_t count = 0;
		for(const auto& material : m_Materials)
		{
			auto offset = m_InstanceData.offset_of(material, name);
			if(offset != std::numeric_limits<decltype(offset)>::max())
			{
				count += set(material, value, offset) ? 1 : 0;
			}
		}

		if(count == 0)
		{
			core::gfx::log->error(
			  "The element name {} was not found in any of the {} materials", name, m_Materials.size());
		}
		return count;
	}

  private:
	bool set(core::resource::tag<core::gfx::geometry_t> geometry,
			 uint32_t id,
			 memory::segment segment,
			 uint32_t size_of_element,
			 const void* data,
			 size_t size,
			 size_t count = 1);

	bool set(core::resource::tag<core::gfx::material_t> material, const void* data, size_t size, size_t offset);

	// ------------------------------------------------------------------------------------------------------------
	// member variables
	// ------------------------------------------------------------------------------------------------------------
  private:
	core::gfx::details::instance::data m_InstanceData;

	psl::array<core::resource::handle<core::gfx::material_t>> m_Materials;
	psl::array<uint32_t> m_Layers;
	psl::UID m_UID;
	core::resource::cache_t& m_Cache;

	core::resource::handle<core::gfx::material_t> m_Bound;
	core::resource::handle<core::gfx::buffer_t> m_MaterialBuffer;
};
}	 // namespace core::gfx
