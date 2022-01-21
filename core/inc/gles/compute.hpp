#pragma once
#include "fwd/gles/compute.hpp"
#include "fwd/gles/texture.hpp"
#include "fwd/resource/resource.hpp"
#include "psl/array_view.hpp"
#include "resource/handle.hpp"

namespace core::data
{
	class material_t;
}

namespace core::igles
{
	class program;
	class program_cache;
	class buffer_t;

	/// \brief core::igles::material_t-like object for compute resources
	///
	/// \details Compute resources encapsulate a core::igles::program that targets the compute stage in the graphics
	/// pipeline. It has a similar to core::igles::material_t like API, and can be considered the equivalent to it, but
	/// instead of targetting draw commands, this encapsulates the abstract compute commands.
	/// Like materials, this object has core::igles::texture_t and core::igles::buffer_t inputs, and instance-like data.
	/// But unlike materials, this object also has core::igles::texture_t and core::igles::buffer_t outputs, which can be
	/// integrated into your normal core::gfx::pass as a (potentially) concurrent process.
	/// The outputs will put a _hard_ sync on draw/compute calls that happen after this that utilize these resources.
	class compute
	{
	  public:
		compute(core::resource::cache_t& cache,
				const core::resource::metadata& metaData,
				psl::meta::file* metaFile,
				core::resource::handle<core::data::material_t> data,
				core::resource::handle<core::igles::program_cache> program_cache);
		~compute();

		compute(const compute& other)	  = delete;
		compute(compute&& other) noexcept = delete;
		compute& operator=(const compute& other) = delete;
		compute& operator=(compute&& other) noexcept = delete;

		void
		dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z = 1u) const noexcept;

		psl::array<core::resource::handle<core::igles::texture_t>> textures() const noexcept;
		psl::array<core::resource::handle<core::igles::buffer_t>> buffers() const noexcept;

	  private:
		psl::meta::file* m_Meta;
		core::resource::handle<core::igles::program> m_Program;
		psl::array<std::pair<uint32_t, core::resource::handle<core::igles::texture_t>>> m_InputTextures;
		psl::array<std::pair<uint32_t, core::resource::handle<core::igles::buffer_t>>> m_InputBuffers;

		psl::array<std::pair<uint32_t, core::resource::handle<core::igles::texture_t>>> m_OutputTextures;
		psl::array<std::pair<uint32_t, core::resource::handle<core::igles::buffer_t>>> m_OutputBuffers;
	};
}	 // namespace core::igles