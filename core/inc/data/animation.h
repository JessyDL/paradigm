#pragma once
#include "fwd/resource/resource.h"
#include "psl/serialization.h"
#include "psl/array.h"
#include "psl/array_view.h"
#include "psl/math/matrix.h"
#include "conversion_utils.h"


namespace core::data
{
	class animation
	{
		friend class psl::serialization::accessor;

		template <typename T, char... Char>
		using prop = psl::serialization::property<T, Char...>;
	  public:
		animation()  = default;
		~animation() = default;

		animation(const animation& other)	 = default;
		animation(animation&& other) noexcept = default;
		animation& operator=(const animation& other) = default;
		animation& operator=(animation&& other) noexcept = default;

	  private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_Name << m_Duration << m_Fps;
		};


		static constexpr const char serialization_name[10]{"ANIMATION"};

		prop<psl::string8_t, const_str("NAME", 4)> m_Name;
		prop<uint32_t, const_str("DURATION", 8)> m_Duration;
		prop<uint32_t, const_str("FPS", 3)> m_Fps;
	};
}