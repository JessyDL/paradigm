#pragma once
#include "resource/resource.hpp"

namespace core::data
{
	class material;
}

namespace memory
{
	class segment;
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
		struct buffer_binding
		{
			core::resource::handle<core::igles::buffer> buffer;
			uint32_t slot;
			uint32_t offset {0};
			uint32_t size {0};
		};

	  public:
		material(core::resource::cache& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 core::resource::handle<core::data::material> data,
				 core::resource::handle<core::igles::program_cache> program_cache);
		~material() = default;

		material(const material& other)		= delete;
		material(material&& other) noexcept = delete;
		material& operator=(const material& other) = delete;
		material& operator=(material&& other) noexcept = delete;

		void bind();

		const std::vector<core::resource::handle<core::igles::shader>>& shaders() const noexcept;

		const core::data::material& data() const noexcept;
		bool bind_instance_data(uint32_t binding, uint32_t offset);

	  private:
		core::resource::handle<program> m_Program;

		core::resource::handle<core::data::material> m_Data;
		std::vector<core::resource::handle<core::igles::shader>> m_Shaders;
		std::vector<std::pair<uint32_t, core::resource::handle<core::igles::texture>>> m_Textures;
		std::vector<std::pair<uint32_t, core::resource::handle<core::igles::sampler>>> m_Samplers;
		std::vector<buffer_binding> m_Buffers;

		// std::optional<std::tuple<uint32_t, core::resource::handle<core::igles::buffer>, memory::segment>>
		// m_MaterialInstanceData{ std::nullopt };
	};
}	 // namespace core::igles