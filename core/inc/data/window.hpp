#pragma once
#include "fwd/resource/resource.hpp"
#include "psl/serialization/serializer.hpp"
#include <cstdint>

namespace core::gfx
{
	/// \brief how many render buffers should there be.
	enum class buffering : uint8_t
	{
		SINGLE = 1,
		DOUBLE = 2,
		TRIPLE = 3,
		COUNT  = TRIPLE
	};

	/// \brief how should the surface be rendered.
	/// \note depending on the platform, fullscreen could be the only option and all others ignored.
	enum class surface_mode : uint8_t
	{
		FULLSCREEN		  = 0,
		WINDOWED		  = 1,
		FULLSCREEN_WINDOW = 2
	};
}	 // namespace core::gfx

namespace core::data
{
	/// \brief contains the data to initialize a core::os::surface
	class window
	{
		friend class psl::serialization::accessor;

	  public:
		window(uint32_t width				  = 800,
			   uint32_t height				  = 600,
			   core::gfx::surface_mode mode	  = core::gfx::surface_mode::WINDOWED,
			   core::gfx::buffering buffering = core::gfx::buffering::SINGLE,
			   psl::string8::view name		  = "Paradigm Engine") noexcept;
		window(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   uint32_t width				  = 800,
			   uint32_t height				  = 600,
			   core::gfx::surface_mode mode	  = core::gfx::surface_mode::WINDOWED,
			   core::gfx::buffering buffering = core::gfx::buffering::SINGLE,
			   psl::string8::view name		  = "Paradigm Engine") noexcept;
		~window()					= default;
		window(const window& other) = default;
		window(window&& other)		= default;
		window& operator=(const window& other) = default;
		window& operator=(window&& other) = default;

		/// \brief returns the width in pixels.
		/// \returns the width in pixels.
		uint32_t width() const;
		/// \brief sets the width in pixels.
		/// \param[in] width the value to set.
		/// \note this value is a suggestion, the actual surface might still have a different value.
		void width(uint32_t width);
		/// \brief returns the height in pixels.
		/// \returns the height in pixels.
		uint32_t height() const;
		/// \brief sets the height in pixels.
		/// \param[in] height the value to set.
		/// \note this value is a suggestion, the actual surface might still have a different value.
		void height(uint32_t height);
		/// \brief returns the name of the surface.
		/// \returns the name of the surface.
		psl::string8::view name() const;
		/// \brief sets the name of the surface.
		/// \param[in] name the name to set this instance's value to.
		/// \note depending on the core::gfx::surface_mode and platform specifics, you might never see
		/// this name anywhere.
		void name(psl::string8::view name);
		/// \brief returns the current mode of rendering.
		/// \returns the current mode of rendering.
		core::gfx::surface_mode mode() const;
		/// \brief sets the current mode of rendering.
		/// \param[in] mode the mode to set this instance's value to.
		void mode(core::gfx::surface_mode mode);
		/// \brief returns the current buffering behaviour of this instance.
		/// \returns the current buffering behaviour of this instance.
		core::gfx::buffering buffering() const;
		/// \brief sets the buffering behaviour of this instance.
		/// \param[in] mode the mode to set this instance's value to.
		void buffering(core::gfx::buffering mode);

	  private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_Name << m_Width << m_Height << m_WindowMode << m_Buffering;
		}

		static constexpr psl::string8::view serialization_name {"WINDOW"};

		psl::serialization::property<"NAME", psl::string8_t> m_Name;
		psl::serialization::property<"WIDTH", uint32_t> m_Width;
		psl::serialization::property<"HEIGHT", uint32_t> m_Height;

		psl::serialization::property<"MODE", core::gfx::surface_mode> m_WindowMode;
		psl::serialization::property<"BUFFERING", core::gfx::buffering> m_Buffering;
	};
}	 // namespace core::data
