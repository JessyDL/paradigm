#pragma once
#include "platform_def.h"

#if defined(SURFACE_XCB)
#ifndef VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#endif
#define VK_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(SURFACE_WAYLAND)
#ifndef VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#define VK_SURFACE_EXTENSION_NAME VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#elif defined(SURFACE_D2D)
#define VK_SURFACE_EXTENSION_NAME VK_KHR_DISPLAY_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#define VK_SURFACE_EXTENSION_NAME VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#elif defined(SURFACE_WIN32)
#define VK_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif

#define VK_VERSION_LATEST_MAJOR 1
#define VK_VERSION_LATEST_MINOR 1
#define VULKAN_HPP_NO_SMART_HANDLE

#ifdef VK_NO_PROTOTYPES
#include "volk.h"
#endif

#include "vulkan/vulkan.hpp"

#define VK_VERSION_LATEST_PATCH VK_HEADER_VERSION
#define VK_API_VERSION_LATEST VK_MAKE_VERSION(VK_VERSION_LATEST_MAJOR, VK_VERSION_LATEST_MINOR, VK_VERSION_LATEST_PATCH)

#include "vulkan_utils.h"

// Set to "true" to use staging buffers for uploading
// vertex and index data to device local memory
// See "prepareVertices" for details on what's staging
// and on why to use it
#define GPU_USE_STAGING true

#ifndef VK_FLAGS_NONE
#define VK_FLAGS_NONE 0
#endif

//#define GAMMA_RENDERING
#ifndef GAMMA_RENDERING
#define SURFACE_FORMAT vk::Format::eB8G8R8A8Srgb
#define SURFACE_COLORSPACE vk::ColorSpaceKHR::eSrgbNonlinear
#else
#define SURFACE_FORMAT vk::Format::eB8G8R8A8Unorm
#define SURFACE_COLORSPACE vk::ColorSpaceKHR::eSrgbNonlinear
#endif

#ifdef DEBUG
#define VULKAN_ENABLE_VALIDATION false
#else
#define VULKAN_ENABLE_VALIDATION false
#endif


namespace utility
{
	template <typename BitType, typename MaskType>
	struct converter<vk::Flags<BitType, MaskType>>
	{
		static psl::string8_t to_string(const vk::Flags<BitType, MaskType>& x)
		{
			return converter<MaskType>::to_string((MaskType)(x));
		}

		static vk::Flags<BitType, MaskType> from_string(psl::string8::view str)
		{
			return (BitType)converter<MaskType>::from_string(str);
		}
	};
} // namespace utility
