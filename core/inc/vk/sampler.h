#pragma once
#include "vulkan_stdafx.h"
#include <vector>
#include "systems/resource.h"

namespace core::data
{
	class sampler;
}

namespace core::ivk
{
	class context;
}

namespace core::gfx
{
	/// \brief sampler object for texture filtering and lookups
	///
	/// In Vulkan textures are accessed by samplers, this separates all the sampling information from the texture data.
	/// this means you could have multiple sampler objects for the same texture with different settings similar to the
	/// samplers available with OpenGL 3.3
	class sampler final
	{
	  public:
		sampler(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<core::ivk::context> context,
				core::resource::handle<core::data::sampler> sampler_data);
		~sampler();

		/// \returns a vulkan sampler object for the given mip chain size.
		/// \param[in] mip the mip chain size to request.
		/// \note it will return mip_max for mip sizes larger than mip_max.
		const vk::Sampler& get(size_t mip = 0u) const noexcept;

		/// \returns the data used by this sampler.
		const core::data::sampler& data() const noexcept;


	  private:
		core::resource::handle<core::data::sampler> m_Data;
		core::resource::handle<core::ivk::context> m_Context;

		std::vector<vk::Sampler> m_Samplers;
	};
} // namespace core::gfx
