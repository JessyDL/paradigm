#pragma once
#include "resource/resource.hpp"

namespace core::data
{
class material_t;
}

namespace memory
{
class segment;
}

namespace core::igles
{
class buffer_t;
class program;
class program_cache;
class texture_t;
class sampler_t;
class shader;

class material_t
{
	struct buffer_binding
	{
		core::resource::handle<core::igles::buffer_t> buffer;
		uint32_t slot;
		uint32_t offset {0};
		uint32_t size {0};
	};

  public:
	material_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::data::material_t> data,
			   core::resource::handle<core::igles::program_cache> program_cache);
	~material_t() = default;

	material_t(const material_t& other)				   = delete;
	material_t(material_t&& other) noexcept			   = delete;
	material_t& operator=(const material_t& other)	   = delete;
	material_t& operator=(material_t&& other) noexcept = delete;

	void bind();

	const std::vector<core::resource::handle<core::igles::shader>>& shaders() const noexcept;

	const core::data::material_t& data() const noexcept;
	bool bind_instance_data(uint32_t binding, uint32_t offset);

  private:
	core::resource::handle<program> m_Program;

	core::resource::handle<core::data::material_t> m_Data;
	std::vector<core::resource::handle<core::igles::shader>> m_Shaders;
	std::vector<std::pair<uint32_t, core::resource::handle<core::igles::texture_t>>> m_Textures;
	std::vector<std::pair<uint32_t, core::resource::handle<core::igles::sampler_t>>> m_Samplers;
	std::vector<buffer_binding> m_Buffers;

	// std::optional<std::tuple<uint32_t, core::resource::handle<core::igles::buffer_t>, memory::segment>>
	// m_MaterialInstanceData{ std::nullopt };
};
}	 // namespace core::igles