#include "core/gles/skeleton.hpp"

namespace core::igles {
skeleton_t::skeleton_t(core::resource::cache_t& cache,
					   const core::resource::metadata& metaData,
					   psl::meta::file* metaFile,
					   core::resource::handle<core::data::skeleton_t> data,
					   core::resource::handle<core::igles::buffer_t> buffer)
	: m_UID(metaData.uid), m_TransformsBuffer(buffer), m_WeightsBuffer(buffer) {
	auto const transforms_size = data->size();
	if(auto segment = m_TransformsBuffer->allocate(transforms_size * sizeof(gpu_transforms_buffer_t)); segment)
	  [[likely]] {
		m_TransformsSegment = segment.value();
	} else {
		core::igles::log->error("Could not allocate '{}' bytes for the {} skeleton_t transforms",
								transforms_size * sizeof(gpu_transforms_buffer_t),
								transforms_size);
		return;
	}

	const size_t weights_size =
	  std::accumulate(std::begin(data->bones()),
					  std::end(data->bones()),
					  size_t {0},
					  [](size_t sum, data::skeleton_t::bone_t const& bone) { return sum + bone.weights().size(); });
	if(auto segment = m_WeightsBuffer->allocate(weights_size * sizeof(gpu_weights_buffer_t)); segment) [[likely]] {
		m_WeightsSegment = segment.value();

	} else {
		core::igles::log->error("Could not allocate '{}' bytes for the {} skeleton_t weights",
								weights_size * sizeof(gpu_weights_buffer_t),
								weights_size);
		return;
	}


	// prepare the gpu data on the cpu, and start the upload
	psl::array<gpu_transforms_buffer_t> gpu_transforms {};
	psl::array<gpu_weights_buffer_t> gpu_weights {};
	gpu_transforms.reserve(transforms_size);
	gpu_weights.reserve(weights_size);

	for(auto const& bone : data->bones()) {
		auto const weight_index = psl::narrow_cast<index_size_t>(gpu_weights.size());
		for(auto const& weight : bone.weights()) {
			gpu_weights.emplace_back(weight.vertex, weight.value);
		}
		gpu_transforms.emplace_back(
		  bone.transform(), weight_index, psl::narrow_cast<index_size_t>(gpu_weights.size()) - weight_index);
	}
	m_TransformsBuffer->set(gpu_transforms.data(),
							{core::gfx::memory_copy {0, m_TransformsSegment.range().begin, transforms_size}});

	m_WeightsBuffer->set(gpu_weights.data(),
						 {core::gfx::memory_copy {0, m_WeightsSegment.range().begin, weights_size}});
}

skeleton_t ::~skeleton_t() {
	if(m_TransformsSegment.is_valid()) [[likely]] {
		m_TransformsBuffer->deallocate(m_TransformsSegment);
		m_TransformsSegment = {};
	}
	if(m_WeightsSegment.is_valid()) [[likely]] {
		m_WeightsBuffer->deallocate(m_WeightsSegment);
		m_WeightsSegment = {};
	}
}
}	 // namespace core::igles
