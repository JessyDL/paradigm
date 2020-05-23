#pragma once
#include "fwd/resource/resource.h"

#include "fwd/vk/texture.h"
#include "fwd/vk/buffer.h"

#include "psl/array.h"
#include "psl/array_view.h"

#include "ivk.h"


namespace core::ivk
{
	class compute
	{
	public:
		compute() = default;
		~compute() = default;

		compute(const compute& other) = default;
		compute(compute&& other) noexcept = default;
		compute& operator=(const compute& other) = default;
		compute& operator=(compute&& other) noexcept = default;

		void dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z = 1u) const
			noexcept;

		psl::array<core::resource::handle<texture>> textures() const noexcept;
		psl::array<core::resource::handle<buffer>> buffers() const noexcept;
	private:

		psl::array<std::pair<uint32_t, core::resource::handle<texture>>> m_InputTextures;
		psl::array<std::pair<uint32_t, core::resource::handle<buffer>>> m_InputBuffers;

		psl::array<std::pair<uint32_t, core::resource::handle<texture>>> m_OutputTextures;
		psl::array<std::pair<uint32_t, core::resource::handle<buffer>>> m_OutputBuffers;

		vk::CommandBuffer m_CommandBuffer;
	};
}