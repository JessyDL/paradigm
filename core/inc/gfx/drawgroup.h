#pragma once

#include "gfx/drawcall.h"
#include "gfx/drawlayer.h"
#include <map>
#include <vector>
#include "systems/resource.h"


namespace core::gfx
{
	class material;
	class geometry;
	class drawgroup;
	class framebuffer;
	class swapchain;
	class drawcall;

	/// \brief a collection of draw instructions to be recorded and sent to the GPU.
	///
	/// describes a group of various core::gfx::drawcalls, ordered by core::gfx::drawlayers.
	/// these are then pinned to a set of possible outputs (swapchain or framebuffer)
	/// which will be used by the render to order and output them.
	class drawgroup
	{
	public:
		drawgroup() = default;
		~drawgroup() = default;
		drawgroup(const drawgroup&) = default;
		drawgroup(drawgroup&&) = default;
		drawgroup& operator=(const drawgroup&) = default;
		drawgroup& operator=(drawgroup&&) = default;

		const drawlayer& layer(const psl::string& layer, uint32_t priority) noexcept;
		bool contains(const psl::string& layer) const noexcept;
		std::optional<std::reference_wrapper<const drawlayer>> get(const psl::string& layer) const noexcept;
		bool priority(drawlayer& layer, uint32_t priority) noexcept;

		bool add(core::resource::indirect_handle<core::gfx::swapchain> swapchain);
		bool add(core::resource::indirect_handle<core::gfx::framebuffer> framebuffer);

		bool remove(core::resource::indirect_handle<core::gfx::swapchain> swapchain);
		bool remove(core::resource::indirect_handle<core::gfx::framebuffer> framebuffer);

		drawcall& add(const drawlayer& layer, core::resource::handle<core::gfx::material> material) noexcept;
		std::optional<std::reference_wrapper<drawcall>> get(const drawlayer& layer, core::resource::handle<core::gfx::material> material) noexcept;

		void build(vk::CommandBuffer cmdBuffer, core::resource::handle<framebuffer> framebuffer, uint32_t index, std::optional<core::resource::handle<core::gfx::material>> replacement = std::nullopt);
		void build(vk::CommandBuffer cmdBuffer, core::resource::handle<swapchain> swapchain, uint32_t index, std::optional<core::resource::handle<core::gfx::material>> replacement = std::nullopt);
		//bool remove(const drawlayer& layer);
		//bool remove(const drawcall& call);
		//bool remove(const drawlayer& layer, const drawcall& call);

	private:
		std::map<drawlayer, std::vector<drawcall>> m_Group;
		core::resource::handle<core::gfx::swapchain> m_Swapchain;
		std::vector<core::resource::handle<core::gfx::framebuffer>> m_Framebuffers;
	};
}
