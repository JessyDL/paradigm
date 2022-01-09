#pragma once
#include "gfx/computecall.h"
#include "gles/types.h"
#include "psl/array_view.h"
#include "psl/view_ptr.h"
#include "resource/resource.hpp"

namespace core::igles
{
	class drawpass;

	class computepass
	{
		struct memory_barrier_t
		{
			GLbitfield barrier {0};
			uint32_t usage {0};
		};

	  public:
		computepass()  = default;
		~computepass() = default;

		computepass(const computepass& other)	  = default;
		computepass(computepass&& other) noexcept = default;
		computepass& operator=(const computepass& other) = default;
		computepass& operator=(computepass&& other) noexcept = default;

		void clear();
		void prepare();
		bool build();
		void present();

		void add(psl::array_view<core::gfx::computecall> compute);
		void add(const core::gfx::computecall& compute);

		void connect(psl::view_ptr<drawpass> pass) noexcept;
		void connect(psl::view_ptr<computepass> pass) noexcept;
		void disconnect(psl::view_ptr<drawpass> pass) noexcept;
		void disconnect(psl::view_ptr<computepass> pass) noexcept;

		psl::array<GLbitfield> memory_barriers() const noexcept;

	  private:
		psl::array<core::gfx::computecall> m_Compute;
		psl::array<memory_barrier_t> m_MemoryBarriers;
	};
}	 // namespace core::igles