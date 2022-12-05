#pragma once

#include "gfx/drawcall.hpp"
#include "gfx/drawlayer.hpp"
#include "resource/resource.hpp"
#include <map>
#include <vector>

#ifdef PE_VULKAN
namespace core::ivk
{
	class drawpass;
}
#endif
#ifdef PE_GLES
namespace core::igles
{
	class drawpass;
}
#endif

namespace core::gfx
{
	class bundle;
	class drawgroup;
	class drawcall;

	/// \brief a collection of draw instructions to be recorded and sent to the GPU.
	///
	/// describes a group of various core::gfx::drawcalls, ordered by core::gfx::drawlayers.
	/// these are then pinned to a set of possible outputs (swapchain or framebuffer)
	/// which will be used by the render to order and output them.
	class drawgroup
	{
#ifdef PE_VULKAN
		friend class core::ivk::drawpass;
#endif
#ifdef PE_GLES
		friend class core::igles::drawpass;
#endif
	  public:
		drawgroup()					= default;
		~drawgroup()				= default;
		drawgroup(const drawgroup&) = default;
		drawgroup(drawgroup&&)		= default;
		drawgroup& operator=(const drawgroup&) = default;
		drawgroup& operator=(drawgroup&&) = default;

		const drawlayer& layer(const psl::string& layer, uint32_t priority, uint32_t extent) noexcept;
		bool contains(const psl::string& layer) const noexcept;
		std::optional<std::reference_wrapper<const drawlayer>> get(const psl::string& layer) const noexcept;
		bool priority(drawlayer& layer, uint32_t priority) noexcept;

		drawcall& add(const drawlayer& layer, core::resource::handle<core::gfx::bundle> bundle) noexcept;
		std::optional<std::reference_wrapper<drawcall>> get(const drawlayer& layer,
															core::resource::handle<core::gfx::bundle> bundle) noexcept;

		// bool remove(const drawlayer& layer);
		// bool remove(const drawcall& call);
		// bool remove(const drawlayer& layer, const drawcall& call);

	  private:
		std::map<drawlayer, std::vector<drawcall>> m_Group;
	};
}	 // namespace core::gfx
