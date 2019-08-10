#pragma once
#include "resource/resource.hpp"

namespace core::igles
{
	class swapchain;

	class pass
	{
	  public:
		pass(core::resource::handle<swapchain> swapchain);
		~pass() = default;

		pass(const pass& other)	 = default;
		pass(pass&& other) noexcept = default;
		pass& operator=(const pass& other) = default;
		pass& operator=(pass&& other) noexcept = default;

		void clear();
		void present();
	  private:
		core::resource::handle<swapchain> m_Swapchain;
	};
}