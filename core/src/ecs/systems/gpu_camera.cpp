#include "core/ecs/systems/gpu_camera.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/transform.hpp"
#include "core/gfx/buffer.hpp"
#include "core/os/surface.hpp"
#include "psl/ecs/state.hpp"

using namespace core::ecs::systems;
using namespace psl;
using namespace psl::math;

#undef near
#undef far


gpu_camera::gpu_camera(psl::ecs::state_t& state,
					   core::resource::handle<core::os::surface> surface,
					   core::resource::handle<core::gfx::shader_buffer_binding> binding,
					   core::gfx::graphics_backend backend)
	: m_Surface(surface), m_Binding(binding), m_Backend(backend) {
	m_Max = m_Binding->segment.range().size() / sizeof(framedata);
	state.declare<"gpu_camera::tick">(psl::ecs::threading::seq, &gpu_camera::tick, this);
}

void gpu_camera::tick(
  psl::ecs::info_t& info,
  psl::ecs::pack_direct_full_t<const core::ecs::components::camera, const core::ecs::components::transform> cameras) {
	size_t i {0};
	if(cameras.size() >= m_Max)
		throw std::runtime_error(
		  fmt::format("cannot allocate more than {}, but {} was requested for this frame", m_Max, cameras.size()));

	auto camera_components = cameras.get<const core::ecs::components::camera>();
	auto transf_components = cameras.get<const core::ecs::components::transform>();

	for(size_t i = 0; i < cameras.size(); ++i) {
		update_buffer(i, transf_components[i], camera_components[i]);
	}
}

void gpu_camera::update_buffer(size_t index,
							   const core::ecs::components::transform& transform,
							   const core::ecs::components::camera& camera) {
	using namespace psl;
	PROFILE_SCOPE(core::profiler)
	vec3 position  = transform.position;
	vec3 direction = transform.rotation * vec3::forward;
	vec3 up		   = vec3::up;

	framedata fdata {};

	fdata.ScreenParams =
	  psl::vec4((float)m_Surface->data().width(), (float)m_Surface->data().height(), camera.near, camera.far);

	fdata.projectionMatrix =
	  math::perspective_projection(math::radians(camera.fov),
								   (float)m_Surface->data().width() / (float)m_Surface->data().height(),
								   camera.near,
								   camera.far);

	if(m_Backend == core::gfx::graphics_backend::gles)
		fdata.projectionMatrix.at<1, 1>() = -fdata.projectionMatrix.at<1, 1>();

	fdata.clipMatrix  = clip;
	fdata.viewMatrix  = look_at(position, position + direction, up);
	fdata.modelMatrix = mat4x4(1);
	fdata.viewPos	  = vec4(position, 1.0);
	fdata.viewDir	  = vec4(direction, 0.0);
	fdata.viewDirQuat = transform.rotation;
	fdata.VP		  = fdata.clipMatrix * fdata.projectionMatrix * fdata.viewMatrix;
	fdata.WVP		  = fdata.VP * fdata.modelMatrix;
	fdata.viewMatrix  = fdata.viewMatrix;
	std::vector<core::gfx::commit_instruction> instructions;
	m_Binding->buffer->commit({core::gfx::commit_instruction {
	  &fdata,
	  m_Binding->segment,
	  memory::range_t {index * m_Binding->region.alignment(), (index + 1) * m_Binding->region.alignment()}}});
}
