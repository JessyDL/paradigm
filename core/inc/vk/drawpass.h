#pragma once
#include "psl/math/vec.hpp"
#include "psl/view_ptr.hpp"
#include "resource/resource.hpp"
#include "vk/ivk.h"
#include <vector>

namespace core::gfx
{
	class drawgroup;
}

namespace core::ivk
{
	class context;
	class framebuffer_t;
	class swapchain;
}	 // namespace core::ivk

namespace core::ivk
{
	struct depth_bias
	{
		union
		{
			union
			{
				float constantFactor;
				float clamp;
				float slopeFactor;
			};
			psl::vec3 bias;
			float components[3];
		};

		depth_bias() : bias(psl::vec3::zero) {};
	};

	/// \brief a pass describes a start-to-finish sequence of commands to render into a set of rendertargets.
	///
	/// A pass is a collection of drawcalls, grouped into sets of drawgroups, and a target framebuffer or swapchain.
	/// This describes a full pipeline (synced) to get something into a set of rendertargets and to either use it in
	/// subsequent passes, or present it to a core::gfx::surface.
	/// Passes also make sure that they don't create race conditions with other passes that have been assigned as its
	/// dependencies.
	class drawpass
	{
	  public:
		/// \brief creates a pass that targets a framebuffer.
		/// \param[in] context the valid and loaded context to bind this pass to.
		/// \param[in] framebuffer the valid and loaded framebuffer that this pass will output to.
		drawpass(core::resource::handle<core::ivk::context> context,
				 core::resource::handle<core::ivk::framebuffer_t> framebuffer);
		/// \brief creates a pass that targets a swapchain image set.
		/// \param[in] context the valid and loaded context to bind this pass to.
		/// \param[in] swapchain the valid and loaded swapchain that this pass will output to.
		drawpass(core::resource::handle<core::ivk::context> context,
				 core::resource::handle<core::ivk::swapchain> swapchain);
		~drawpass();
		drawpass(const drawpass&) = delete;
		drawpass(drawpass&&)	  = delete;
		drawpass& operator=(const drawpass&) = delete;
		drawpass& operator=(drawpass&&) = delete;

		/// \brief prepares the pass for presenting (i.e. it does some basic housekeeping such as fetching the swapchain
		/// image if any)
		void prepare();

		/// \brief submits the recorded instructions to the GPU, handling the dependencies accordingly.
		void present();

		/// \brief set the depth bias on the current pass.
		void bias(const core::ivk::depth_bias& bias) noexcept;

		/// \brief returns the current dept bias on this instance.
		/// \returns the current dept bias on this instance.
		core::ivk::depth_bias bias() const noexcept;

		/// \brief build, and records the draw, and other instructions associated with this pass.
		/// \returns true on success, false if submitting the instructions to the GPU failed.
		bool build();

		/// \brief add an additional drawgroup to be included in this pass' draw instructions.
		void add(core::gfx::drawgroup& group) noexcept;

		/// \brief makes the current pass wait for the given pass to complete
		void connect(psl::view_ptr<drawpass> pass) noexcept;

		void disconnect(psl::view_ptr<drawpass> pass) noexcept;

		/// \brief removes an existing drawgroup from this pass' draw instructions.
		void remove(const core::gfx::drawgroup& group) noexcept;

		/// \brief removes all drawgroups from this pass' draw instructions.
		void clear() noexcept;

		bool is_swapchain() const noexcept;

	  private:
		void build_drawgroup(core::gfx::drawgroup& group,
							 vk::CommandBuffer cmdBuffer,
							 core::resource::handle<core::ivk::framebuffer_t> framebuffer,
							 uint32_t index);
		void build_drawgroup(core::gfx::drawgroup& group,
							 vk::CommandBuffer cmdBuffer,
							 core::resource::handle<core::ivk::swapchain> swapchain,
							 uint32_t index);
		/// \brief creates the vk::Fence's that will be used to sync access to this pass.
		/// \param[in] size the amount of fences to create.
		void create_fences(const size_t size = 1u);
		/// \brief cleans up the created vk::Fence's of the core::ivk::pass::create_fences() method.
		void destroy_fences();

		core::resource::handle<core::ivk::context> m_Context;
		core::resource::handle<core::ivk::framebuffer_t> m_Framebuffer;
		core::resource::handle<core::ivk::swapchain> m_Swapchain;
		const bool m_UsingSwap;
		std::vector<core::gfx::drawgroup> m_AllGroups;

		std::vector<vk::Semaphore> m_WaitFor;
		vk::Semaphore m_PresentComplete;
		vk::Semaphore m_RenderComplete;
		std::vector<vk::Fence> m_WaitFences;

		std::vector<vk::CommandBuffer> m_DrawCommandBuffers;
		uint32_t m_Buffers;
		uint32_t m_CurrentBuffer;
		uint64_t m_FrameCount {0u};
		uint64_t m_LastBuildFrame {0};

		core::ivk::depth_bias m_DepthBias;

		// Contains command buffers and semaphores to be presented to the queue
		vk::SubmitInfo m_SubmitInfo;
		/** @brief Pipeline stages used to wait at for graphics queue submissions */
		vk::PipelineStageFlags m_SubmitPipelineStages {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	};
}	 // namespace core::ivk
