#include "ecs/systems/gpu_camera.h"
#include "ecs/components/camera.h"
#include "ecs/components/transform.h"
#include "vk/buffer.h"
#include "os/surface.h"
#include "ecs/state.h"
#include "gfx/pass.h"

using namespace core::ecs::systems;
using namespace psl;
using namespace psl::math;

#undef near
#undef far


gpu_camera::gpu_camera(psl::ecs::state& state, core::resource::handle<core::os::surface> surface, core::resource::handle<core::gfx::buffer> buffer)	:	m_Surface(surface), m_Buffer(buffer)
{
	state.declare(psl::ecs::threading::seq, &gpu_camera::tick, this);
}

void gpu_camera::tick(
	psl::ecs::info& info,
	psl::ecs::pack<const core::ecs::components::camera, const core::ecs::components::transform> cameras)
{
	size_t i{0};
	for(auto [camera, transform] : cameras)
	{
		update_buffer(i++, transform, camera);
	}
}

void gpu_camera::update_buffer(size_t index, const core::ecs::components::transform& transform,
							   const core::ecs::components::camera& camera)
{
	using namespace psl;
	PROFILE_SCOPE(core::profiler)
	while(index >= fdatasegment.size())
	{
		fdatasegment.emplace_back(m_Buffer->reserve(sizeof(framedata)).value());
	}
	{
		vec3 position  = transform.position;
		vec3 direction = transform.rotation * vec3::forward;
		vec3 up		   = vec3::up;

		framedata fdata{};
		fdata.ScreenParams =
			psl::vec4((float)m_Surface->data().width(), (float)m_Surface->data().height(), camera.near, camera.far);

		fdata.projectionMatrix = math::perspective_projection(
			math::radians(camera.fov), (float)m_Surface->data().width() / (float)m_Surface->data().height(),
			camera.near, camera.far);
		fdata.clipMatrix = clip;

		fdata.viewMatrix  = math::look_at(position, position + direction, up);
		fdata.modelMatrix = mat4x4(1);
		fdata.viewPos	 = vec4(position, 1.0);
		fdata.viewDir	 = vec4(direction, 0.0);

		fdata.VP  = fdata.clipMatrix * fdata.projectionMatrix * fdata.viewMatrix;
		fdata.WVP = fdata.VP * fdata.modelMatrix;
		std::vector<core::gfx::buffer::commit_instruction> instructions;
		instructions.emplace_back(&fdata, fdatasegment[index]);
		m_Buffer->commit(instructions);
	}
}