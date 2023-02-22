#pragma once
#include "core/data/skeleton.hpp"
#include "core/gles/buffer.hpp"
#include "core/resource/handle.hpp"
#include <psl/math/matrix.hpp>
#include <psl/meta.hpp>

namespace core::data {
class skeleton_t;
}

namespace core::igles {
class skeleton_t {
	using index_size_t = core::data::skeleton_t::index_size_t;
	using weight_t	   = core::data::skeleton_t::weight_t;
	/// @brief Used to describe the buffer layout and appearance on the GPU, this only exists as an assistance tool
	struct gpu_transforms_buffer_t {
		constexpr gpu_transforms_buffer_t() noexcept = default;
		constexpr gpu_transforms_buffer_t(psl::mat4x4 transform_v,
										  index_size_t weight_index_v,
										  index_size_t weight_count_v) noexcept
			: transform(transform_v), weight_index(weight_index_v), weight_count(weight_count_v) {}
		psl::mat4x4 transform;
		/// @brief Refers to the index into the `gpu_weights_buffer_t` what vertices will be associated with this entry
		index_size_t weight_index;
		/// @brief Amount of entries after `weight_index` that are part of this given bone. The range is [inclusive,
		/// exclusive)
		index_size_t weight_count;
	};

	/// @brief Used to describe the buffer layout and appearance on the GPU
	struct gpu_weights_buffer_t {
		constexpr gpu_weights_buffer_t() noexcept = default;
		constexpr gpu_weights_buffer_t(index_size_t vertex_v, weight_t weight_v) noexcept
			: vertex(vertex_v), weight(weight_v) {};

		index_size_t vertex;
		weight_t weight;
	};

  public:
	skeleton_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::resource::handle<core::data::skeleton_t> data,
			   core::resource::handle<core::igles::buffer_t> buffer);

	~skeleton_t();

  private:
	psl::UID m_UID {};
	core::resource::handle<core::igles::buffer_t> m_TransformsBuffer {};
	core::resource::handle<core::igles::buffer_t> m_WeightsBuffer {};
	memory::segment m_TransformsSegment;
	memory::segment m_WeightsSegment;
};
}	 // namespace core::igles
