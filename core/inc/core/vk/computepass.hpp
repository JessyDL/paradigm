#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/view_ptr.hpp"

namespace core::gfx {
class computecall;
}

namespace core::ivk {
class drawpass;
class computepass {
  public:
	computepass()  = default;
	~computepass() = default;

	computepass(const computepass& other)				 = delete;
	computepass(computepass&& other) noexcept			 = delete;
	computepass& operator=(const computepass& other)	 = delete;
	computepass& operator=(computepass&& other) noexcept = delete;

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

  private:
	psl::array<core::gfx::computecall> m_Compute;
};
}	 // namespace core::ivk
