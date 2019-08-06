#pragma once
#include "ustring.h"
#include "meta.h"
#include "resource/resource.hpp"
#include <optional>
#include "vulkan_stdafx.h"

namespace core::meta
{
	class shader;
}

namespace core::ivk
{
	class context;
}

namespace core::ivk
{
	/// \brief creates a shader object from a SPIR-V module
	///
	/// handles loading a SPIR-V object from a source (disk, or otherwise).
	/// and subsequently uploads it to the driver to be validated.
	/// a couple of shaders together with a core::gfx::pipeline and core::ivk::geometry is all that's
	/// needed to render an object on screen.
	class shader
	{
	  public:
		/// \brief contains the specialization info that might be used by the shader.
		struct specialization
		{
			psl::string8_t name;
			specialization() : name(u8"main"){};
			specialization(const specialization& other) : name(other.name) {}

			bool operator==(const specialization& other) const { return name == other.name; }
			bool operator!=(const specialization& other) const { return name != other.name; }
		};
		shader(const psl::UID& uid, core::resource::cache& cache, psl::meta::file* metaFile,
			   core::resource::handle<core::ivk::context> context);
		shader(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<core::ivk::context> context,
			   const std::vector<specialization> specializations);
		~shader();
		shader(const shader&) = delete;
		shader(shader&&)	  = delete;
		shader& operator=(const shader&) = delete;
		shader& operator=(shader&&) = delete;

		/// \returns the pipeline shader stage create info for the given specialization.
		/// \note will return nothing in case the specialization does not exist.
		std::optional<vk::PipelineShaderStageCreateInfo> pipeline(const specialization& description = specialization{});

		/// \returns the meta data that describes this shader and its binding points.
		core::meta::shader* meta() const noexcept;

	  private:
		core::resource::handle<core::ivk::context> m_Context;
		std::vector<std::pair<specialization, vk::PipelineShaderStageCreateInfo>> m_Specializations;
		core::resource::cache& m_Cache;
		core::meta::shader* m_Meta;
		const psl::UID m_UID;
	};
} // namespace core::gfx
