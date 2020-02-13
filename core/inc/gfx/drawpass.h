#pragma once
#include "resource/resource.hpp"
#include <variant>
#include "psl/view_ptr.h"
#ifdef PE_GLES
namespace core::igles
{
	class drawpass;
}
#endif
#ifdef PE_VULKAN
namespace core::ivk
{
	class drawpass;
}
#endif

namespace core::gfx
{
	class context;
	class framebuffer;
	class swapchain;
	class drawgroup;
	class computecall;
	class computepass;

	class drawpass
	{
		using value_type = std::variant<
#ifdef PE_VULKAN
			core::ivk::drawpass*
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::drawpass*
#endif
			>;
	  public:
		drawpass(core::resource::handle<context> context, core::resource::handle<framebuffer> framebuffer);
		drawpass(core::resource::handle<context> context, core::resource::handle<swapchain> swapchain);
		~drawpass();

		drawpass(const drawpass& other)		= delete;
		drawpass(drawpass&& other) noexcept = delete;
		drawpass& operator=(const drawpass& other) = delete;
		drawpass& operator=(drawpass&& other) noexcept = delete;

		bool is_swapchain() const noexcept;

		
		void clear();
		void prepare();
		bool build(bool force = false);
		void present();

		bool connect(psl::view_ptr<core::gfx::drawpass> child) noexcept;
		bool connect(psl::view_ptr<core::gfx::computepass> child) noexcept { return true; };
		bool disconnect(psl::view_ptr<core::gfx::drawpass> child) noexcept;
		bool disconnect(psl::view_ptr<core::gfx::computepass> child) noexcept { return true; };
		void add(core::gfx::drawgroup& group) noexcept;

		value_type resource() const noexcept { return m_Handle; };

		void dirty(bool value) noexcept { m_Dirty = value; }
		bool dirty() const noexcept { return m_Dirty; }
	  private:
		value_type m_Handle;
		bool m_Dirty{true};
	};
} // namespace core::gfx