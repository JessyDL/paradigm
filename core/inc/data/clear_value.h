#pragma once
#include "array.h"
#include "vulkan_stdafx.h"
#include "serialization.h"

namespace core::data
{
	/// \brief typesafe alternative to vk::ClearValue
	class clear_value
	{
		struct depth_stencil
		{
			float depth;
			uint32_t stencil;
		};


		template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
		template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	  public:
		clear_value(psl::static_array<float, 4> colorValue) : m_ClearValue(colorValue), m_Mode(1){};
		clear_value(psl::static_array<uint32_t, 4> colorValue) : m_ClearValue(colorValue), m_Mode(2){};
		clear_value(psl::static_array<int32_t, 4> colorValue) : m_ClearValue(colorValue), m_Mode(3){};
		clear_value(float depth, uint32_t stencil) : m_ClearValue(depth_stencil{depth, stencil}), m_Mode(4){};

		operator vk::ClearValue() const noexcept
		{

		}

		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_Mode;
			if constexpr(psl::serialization::details::is_decoder<S>::value)
			{
				switch(m_Mode.value)
				{
				case 1:
				{
					psl::serialization::property<float[4], const_str("VALUE", 5)> value{};
					serializer << value;
					m_ClearValue = psl::static_array<float, 4>{value};
				}
				break;
				case 2:
				{
					psl::serialization::property<uint32_t[4], const_str("VALUE", 5)> value{};
					serializer << value;
					m_ClearValue = psl::static_array<uint32_t, 4>{value};
				}
				break;
				case 3:
				{
					psl::serialization::property<int32_t[4], const_str("VALUE", 5)> value{};
					serializer << value;
					m_ClearValue = psl::static_array<int32_t, 4>{value};
				}
				break;
				case 4:
				{
					psl::serialization::property<float, const_str("DEPTH", 5)> depth{};
					psl::serialization::property<uint32_t, const_str("STENCIL", 7)> stencil{};
					serializer << depth << stencil;
					m_ClearValue = depth_stencil{depth, stencil};
				}
				break;
				default: break;
				}
			}
			else
			{
				std::visit(m_ClearValue,
						   [](psl::static_array<float, 4>& val) {
							   psl::serialization::property<float[4], const_str("VALUE", 5)> value{val};
							   serializer << value;
						   },
						   [](psl::static_array<uint32_t, 4>& val) {
							   psl::serialization::property<uint32_t[4], const_str("VALUE", 5)> value{val};
							   serializer << value;
						   },
						   [](psl::static_array<int32_t, 4>& val) {
							   psl::serialization::property<int32_t[4], const_str("VALUE", 5)> value{val};
							   serializer << value;
						   },
						   [](depth_stencil& val) {
							   psl::serialization::property<float, const_str("DEPTH", 5)> depth{val.depth};
							   psl::serialization::property<uint32_t, const_str("STENCIL", 7)> stencil{val.stencil};
							   serializer << depth << stencil;
						   });
			}
		}

	  private:
		std::variant<psl::static_array<float, 4>, psl::static_array<uint32_t, 4>, psl::static_array<int32_t, 4>,
					 depth_stencil>
			m_ClearValue;
		psl::serialization::property<int, const_str("MODE", 4)> m_Mode;
	};

} // namespace core::data