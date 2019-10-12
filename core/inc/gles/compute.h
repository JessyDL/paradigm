#pragma once
#include "fwd/resource/resource.h"
#include "resource/handle.h"
#include "fwd/gles/compute.h"
#include "fwd/gles/texture.h"
#include "psl/array_view.h"

namespace core::data
{
	class material;
}

namespace core::igles
{
	class program;
	class program_cache;
	class buffer;

	/// \brief core::igles::material-like object for compute resources
	///
	/// \details Compute resources encapsulate a core::igles::program that targets the compute stage in the graphics
	/// pipeline. It has a similar to core::igles::material like API, and can be considered the equivalent to it, but
	/// instead of targetting draw commands, this encapsulates the abstract compute commands.
	/// Like materials, this object has core::igles::texture and core::igles::buffer inputs, and instance-like data.
	/// But unlike materials, this object also has core::igles::texture and core::igles::buffer outputs, which can be
	/// integrated into your normal core::gfx::pass as a (potentially) concurrent process.
	/// The outputs will put a _hard_ sync on draw/compute calls that happen after this that utilize these resources.
	class compute
	{
	  public:
		compute(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile,
				core::resource::handle<core::data::material> data,
				core::resource::handle<core::igles::program_cache> program_cache);
		~compute();

		compute(const compute& other)	 = delete;
		compute(compute&& other) noexcept = delete;
		compute& operator=(const compute& other) = delete;
		compute& operator=(compute&& other) noexcept = delete;

		void dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z = 1u) const
			noexcept;

		psl::array<core::resource::handle<core::igles::texture>> textures() const noexcept;
		psl::array<core::resource::handle<core::igles::buffer>> buffers() const noexcept;
	  private:
		psl::meta::file* m_Meta;
		core::resource::handle<core::igles::program> m_Program;
		psl::array<std::pair<uint32_t, core::resource::handle<core::igles::texture>>> m_InputTextures;
		psl::array<std::pair<uint32_t, core::resource::handle<core::igles::buffer>>> m_InputBuffers;

		psl::array<std::pair<uint32_t, core::resource::handle<core::igles::texture>>> m_OutputTextures;
		psl::array<std::pair<uint32_t, core::resource::handle<core::igles::buffer>>> m_OutputBuffers;
	};
} // namespace core::igles