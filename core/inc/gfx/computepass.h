#pragma once
#include <variant>
#include "psl/view_ptr.h"
#include "resource/resource.hpp"

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

	/// \brief describes a compute stage in the render_graph
	class computepass
	{
		using value_type = std::variant<
#ifdef PE_VULKAN
			core::ivk::computepass*
#ifdef PE_GLES
			,
#endif
#endif
#ifdef PE_GLES
			core::igles::computepass*
#endif
			>;
	  public:
		computepass(core::resource::handle<context> context);
		~computepass();

		computepass(const computepass& other)	 = delete;
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
		value_type m_Handle;
		bool m_Dirty{false};
	};
} // namespace core::gfx