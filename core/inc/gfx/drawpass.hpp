#pragma once
#include "gfx/types.hpp"
#include "psl/view_ptr.hpp"
#include "resource/resource.hpp"
#include <variant>
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
	class framebuffer_t;
	class swapchain;
	class drawgroup;
	class computecall;
	class computepass;

	class drawpass;

#ifdef PE_VULKAN
	template <>
	struct backend_type<drawpass, graphics_backend::vulkan>
	{
		using type = core::ivk::drawpass;
	};
#endif
#ifdef PE_GLES
	template <>
	struct backend_type<drawpass, graphics_backend::gles>
	{
		using type = core::igles::drawpass;
	};
#endif

	class drawpass
	{
	  public:
#ifdef PE_VULKAN
		explicit drawpass(core::ivk::drawpass* handle);
#endif
#ifdef PE_GLES
		explicit drawpass(core::igles::drawpass* handle);
#endif

		drawpass(core::resource::handle<context> context, core::resource::handle<framebuffer_t> framebuffer);
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

		void dirty(bool value) noexcept { m_Dirty = value; }
		bool dirty() const noexcept { return m_Dirty; }
		template <core::gfx::graphics_backend backend>
		backend_type_t<drawpass, backend>* resource() const noexcept
		{
#ifdef PE_VULKAN
			if constexpr(backend == graphics_backend::vulkan) return m_VKHandle;
#endif
#ifdef PE_GLES
			if constexpr(backend == graphics_backend::gles) return m_GLESHandle;
#endif
		};

	  private:
		core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
		core::ivk::drawpass* m_VKHandle {nullptr};
#endif
#ifdef PE_GLES
		core::igles::drawpass* m_GLESHandle {nullptr};
#endif
		bool m_Dirty {true};
	};
}	 // namespace core::gfx