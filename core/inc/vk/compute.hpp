#pragma once
#include "fwd/resource/resource.hpp"

#include "fwd/vk/buffer.hpp"
#include "fwd/vk/texture.hpp"

#include "psl/array.hpp"
#include "psl/array_view.hpp"

#include "ivk.hpp"


namespace core::ivk
{
class compute
{
  public:
	compute()  = default;
	~compute() = default;

	compute(const compute& other)				 = default;
	compute(compute&& other) noexcept			 = default;
	compute& operator=(const compute& other)	 = default;
	compute& operator=(compute&& other) noexcept = default;

	void dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z = 1u) const noexcept;

	psl::array<core::resource::handle<texture_t>> textures() const noexcept;
	psl::array<core::resource::handle<buffer_t>> buffers() const noexcept;

  private:
	psl::array<std::pair<uint32_t, core::resource::handle<texture_t>>> m_InputTextures;
	psl::array<std::pair<uint32_t, core::resource::handle<buffer_t>>> m_InputBuffers;

	psl::array<std::pair<uint32_t, core::resource::handle<texture_t>>> m_OutputTextures;
	psl::array<std::pair<uint32_t, core::resource::handle<buffer_t>>> m_OutputBuffers;

	vk::CommandBuffer m_CommandBuffer;
};
}	 // namespace core::ivk