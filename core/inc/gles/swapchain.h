#pragma once
#include "psl/math/vec.h"
#include "resource/handle.h"

namespace core::os
{
	class surface;
}

namespace core::igles
{
	class context;
	class swapchain
	{
	  public:
		swapchain(core::resource::cache& cache,
				  const core::resource::metadata& metaData,
				  psl::meta::file* metaFile,
				  core::resource::handle<core::os::surface> surface,
				  core::resource::handle<core::igles::context> context,
				  bool use_depth = true);
		~swapchain();

		swapchain(const swapchain& other)	  = default;
		swapchain(swapchain&& other) noexcept = default;
		swapchain& operator=(const swapchain& other) = default;
		swapchain& operator=(swapchain&& other) noexcept = default;

		bool present();
		void clear();
		/*/// returns the amount of images in the swapchain
		uint32_t size() const noexcept;

		/// \returns the width of the swapchain image
		uint32_t width() const noexcept;
		/// \returns the height of the swapchainimage.
		uint32_t height() const noexcept;

		/// \returns the clear color value assigned to the swapchain.
		const psl::vec4 clear_color() const noexcept;


		/// \returns the clear depth color of the swapchain
		/// \warning will return default value in case there is no depth texture.
		const float clear_depth() const noexcept;

		/// \returns the clear depth color of the swapchain
		/// \warning will return default value in case there is no depth texture.
		const uint32_t clear_stencil() const noexcept;

		/// \returns if the swapchain uses a depth texture or not.
		bool has_depth() const noexcept;

		/// \brief sets the clear color
		/// \param[in] color the new clear color value to use
		void clear_color(psl::vec4 color) noexcept;

		/// \returns false in case the window might be resizing.
		bool is_ready() const noexcept;*/

	  private:
		psl::vec4 m_ClearColor {0.25f, 0.4f, 0.95f, 1.0f};
		float m_ClearDepth {1.0f};
		uint32_t m_ClearStencil {0};
		bool m_UseDepth {false};
		core::resource::handle<core::os::surface> m_Surface;
		core::resource::handle<core::igles::context> m_Context;
	};
}	 // namespace core::igles