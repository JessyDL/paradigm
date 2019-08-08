#pragma once

#include "gfx/drawcall.h"
#include "gfx/drawlayer.h"
#include <map>
#include <vector>
#include "resource/resource.hpp"

namespace core::ivk
{
	class framebuffer;
	class swapchain;
	class geometry;
}

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

		bool add(core::resource::indirect_handle<core::ivk::swapchain> swapchain);
		bool add(core::resource::indirect_handle<core::ivk::framebuffer> framebuffer);

		bool remove(core::resource::indirect_handle<core::ivk::swapchain> swapchain);
		bool remove(core::resource::indirect_handle<core::ivk::framebuffer> framebuffer);

		drawcall& add(const drawlayer& layer, core::resource::handle<core::gfx::bundle> bundle) noexcept;
		std::optional<std::reference_wrapper<drawcall>> get(const drawlayer& layer,
															core::resource::handle<core::gfx::bundle> bundle) noexcept;

		void build(vk::CommandBuffer cmdBuffer, core::resource::handle<core::ivk::framebuffer> framebuffer,
				   uint32_t index);
		void build(vk::CommandBuffer cmdBuffer, core::resource::handle<core::ivk::swapchain> swapchain, uint32_t index);
		// bool remove(const drawlayer& layer);
		// bool remove(const drawcall& call);
		// bool remove(const drawlayer& layer, const drawcall& call);

	  private:
		std::map<drawlayer, std::vector<drawcall>> m_Group;
		core::resource::handle<core::ivk::swapchain> m_Swapchain;
		std::vector<core::resource::handle<core::ivk::framebuffer>> m_Framebuffers;
	};
} // namespace core::gfx
