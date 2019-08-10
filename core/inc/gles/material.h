#pragma once
#include "resource/resource.hpp"

namespace core::data
{
	class material;
}
namespace core::igles
{
	class buffer;
	class program;
	class program_cache;
	class texture;
	class sampler;
	class shader;

	class material
	{
	  public:
		material(const psl::UID& uid, core::resource::cache& cache, core::resource::handle<core::data::material> data,
				 core::resource::handle<core::igles::program_cache> program_cache,
				 core::resource::handle<core::igles::buffer> materialBuffer);
		~material() = default;

		material(const material& other)		= delete;
		material(material&& other) noexcept = delete;
		material& operator=(const material& other) = delete;
		material& operator=(material&& other) noexcept = delete;

		void bind();

		const std::vector<core::resource::handle<core::igles::shader>>& shaders() const noexcept;

		const core::data::material& data() const noexcept;
	  private:
		core::resource::handle<program> m_Program;

		core::resource::handle<core::data::material> m_Data;
		std::vector<core::resource::handle<core::igles::shader>> m_Shaders;
		std::vector<std::pair<uint32_t, core::resource::handle<core::igles::texture>>> m_Textures;
		std::vector<std::pair<uint32_t, core::resource::handle<core::igles::sampler>>> m_Samplers;
		std::vector<std::pair<uint32_t, core::resource::handle<core::igles::buffer>>> m_Buffers;
	};
} // namespace core::igles