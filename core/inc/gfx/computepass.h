#pragma once
#include "gfx/types.h"
#include "psl/view_ptr.hpp"
#include "resource/resource.hpp"
#include <variant>

#ifdef PE_GLES
namespace core::igles
{
	class computepass;
}
#endif
#ifdef PE_VULKAN
namespace core::ivk
{
	class computepass;
}
#endif
namespace core::gfx
{
	class context;
	class drawpass;
	class computecall;


	class computepass;

#ifdef PE_VULKAN
	template <>
	struct backend_type<computepass, graphics_backend::vulkan>
	{
		using type = core::ivk::computepass;
	};
#endif
#ifdef PE_GLES
	template <>
	struct backend_type<computepass, graphics_backend::gles>
	{
		using type = core::igles::computepass;
	};
#endif

	/// \brief describes a compute stage in the render_graph
	class computepass
	{
	  public:
#ifdef PE_VULKAN
		computepass(core::ivk::computepass* handle);
#endif
#ifdef PE_GLES
		computepass(core::igles::computepass* handle);
#endif
		computepass(core::resource::handle<context> context);
		~computepass();

		computepass(const computepass& other)	  = delete;
		computepass(computepass&& other) noexcept = delete;
		computepass& operator=(const computepass& other) = delete;
		computepass& operator=(computepass&& other) noexcept = delete;


		void clear();
		void prepare();
		bool build(bool force = false);
		void present();

		bool connect(psl::view_ptr<core::gfx::drawpass> child) noexcept { return true; };
		bool connect(psl::view_ptr<core::gfx::computepass> child) noexcept;
		bool disconnect(psl::view_ptr<core::gfx::drawpass> child) noexcept { return true; };
		bool disconnect(psl::view_ptr<core::gfx::computepass> child) noexcept;

		void add(const core::gfx::computecall& call) noexcept;

		void dirty(bool value) noexcept { m_Dirty = value; }
		bool dirty() const noexcept { return m_Dirty; }

	  private:
		core::gfx::graphics_backend m_Backend {graphics_backend::undefined};
#ifdef PE_VULKAN
		core::ivk::computepass* m_VKHandle;
#endif
#ifdef PE_GLES
		core::igles::computepass* m_GLESHandle;
#endif
		bool m_Dirty {false};
	};
}	 // namespace core::gfx