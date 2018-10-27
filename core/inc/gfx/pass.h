#pragma once
#include "stdafx.h"

namespace core::gfx
{
	class context;
	class framebuffer;
	class swapchain;
	class drawgroup;
	struct depth_bias
	{
		union
		{
			struct
			{
				float constantFactor;
				float clamp;
				float slopeFactor;
			};
			glm::vec3 bias;
			float components[3];
		};

		depth_bias() : bias(glm::vec3::Zero){};
	};

	/// \brief a pass describes a start-to-finish sequence of commands to render into a set of rendertargets.
	///
	/// A pass is a collection of drawcalls, grouped into sets of drawgroups, and a target framebuffer or swapchain.
	/// This describes a full pipeline (synced) to get something into a set of rendertargets and to either use it in
	/// subsequent passes, or present it to a core::gfx::surface.
	/// Passes also make sure that they don't create race conditions with other passes that have been assigned as its dependencies.
	class pass
	{
	  public:
		  /// \brief creates a pass that targets a framebuffer.
		  /// \param[in] context the valid and loaded context to bind this pass to.
		  /// \param[in] framebuffer the valid and loaded framebuffer that this pass will output to.
		pass(core::resource::handle<core::gfx::context> context,
			 core::resource::handle<core::gfx::framebuffer> framebuffer);
		/// \brief creates a pass that targets a swapchain image set.
		/// \param[in] context the valid and loaded context to bind this pass to.
		/// \param[in] swapchain the valid and loaded swapchain that this pass will output to.
		pass(core::resource::handle<core::gfx::context> context,
			 core::resource::handle<core::gfx::swapchain> swapchain);
		~pass();
		pass(const pass&) = delete;
		pass(pass&&)	  = delete;
		pass& operator=(const pass&) = delete;
		pass& operator=(pass&&) = delete;

		/// \brief prepares the pass for presenting (i.e. it does some basic housekeeping such as fetching the swapchain image if any)
		void prepare();

		/// \brief submits the recorded instructions to the GPU, handling the dependencies accordingly.
		void present();

		/// \brief set the depth bias on the current pass.
		void bias(const core::gfx::depth_bias& bias);

		/// \brief returns the current dept bias on this instance.
		/// \returns the current dept bias on this instance.
		core::gfx::depth_bias bias() const;

		/// \brief build, and records the draw, and other instructions associated with this pass.
		/// \returns true on success, false if submitting the instructions to the GPU failed.
		bool build();

		/// \brief add an additional drawgroup to be included in this pass' draw instructions.
		void add(core::gfx::drawgroup& group);

		/// \brief removes an existing drawgroup from this pass' draw instructions.
		void remove(const core::gfx::drawgroup& group);

	  private:
		/// \brief creates the vk::Fence's that will be used to sync access to this pass.
		/// \param[in] size the amount of fences to create.
		void create_fences(const size_t size = 1u);
		/// \brief cleans up the created vk::Fence's of the core::gfx::pass::create_fences() method.
		void destroy_fences();

		core::resource::handle<core::gfx::context> m_Context;
		core::resource::handle<core::gfx::framebuffer> m_Framebuffer;
		core::resource::handle<core::gfx::swapchain> m_Swapchain;
		const bool m_UsingSwap;
		std::vector<core::gfx::drawgroup> m_AllGroups;

		vk::Semaphore m_PresentComplete;
		vk::Semaphore m_RenderComplete;
		std::vector<vk::Fence> m_WaitFences;

		std::vector<vk::CommandBuffer> m_DrawCommandBuffers;
		uint32_t m_Buffers;
		uint32_t m_CurrentBuffer;
		uint64_t m_FrameCount{0u};
		uint64_t m_LastBuildFrame{0};

		core::gfx::depth_bias m_DepthBias;

		// Contains command buffers and semaphores to be presented to the queue
		vk::SubmitInfo m_SubmitInfo;
		/** @brief Pipeline stages used to wait at for graphics queue submissions */
		vk::PipelineStageFlags m_SubmitPipelineStages{vk::PipelineStageFlagBits::eColorAttachmentOutput};
	};
} // namespace core::gfx
