#pragma once
#include "core/data/geometry.hpp"
#include "psl/serialization/property.hpp"
#include "psl/serialization/serializer.hpp"
#include "psl/utility/enum.hpp"

namespace core::data {
class animation_t {
	friend class psl::serialization::accessor;
	static constexpr psl::string8::view serialization_name {"ANIMATION"};

  public:
	using index_size_t = core::data::geometry_t::index_size_t;
	using precision_t  = double;

	struct bone_t {
	  private:
		friend class psl::serialization::accessor;
		static constexpr psl::string8::view serialization_name {"ANIMATION_BONE"};

	  public:
		template <typename T>
		struct timestamped_value_t {
			static constexpr psl::string8::view serialization_name {"ANIMATION_TIMESTAMPED_VALUE"};

			friend constexpr bool operator<(timestamped_value_t const& lhs, timestamped_value_t const& rhs) noexcept {
				return lhs.time < rhs.time;
			}
			friend constexpr bool operator<=(timestamped_value_t const& lhs, timestamped_value_t const& rhs) noexcept {
				return lhs.time <= rhs.time;
			}
			friend constexpr bool operator>(timestamped_value_t const& lhs, timestamped_value_t const& rhs) noexcept {
				return lhs.time > rhs.time;
			}
			friend constexpr bool operator>=(timestamped_value_t const& lhs, timestamped_value_t const& rhs) noexcept {
				return lhs.time >= rhs.time;
			}
			friend constexpr bool operator!=(timestamped_value_t const& lhs, timestamped_value_t const& rhs) noexcept {
				return lhs.time != rhs.time;
			}
			friend constexpr bool operator==(timestamped_value_t const& lhs, timestamped_value_t const& rhs) noexcept {
				return lhs.time == rhs.time;
			}

			template <typename S>
			void serialize(S& serializer) {
				serializer << data << time;
			}
			psl::serialization::property<"VALUE", T> data {};
			psl::serialization::property<"TIME", precision_t> time {};
		};

		using position_timed_t = timestamped_value_t<psl::vec3>;
		using rotation_timed_t = timestamped_value_t<psl::quat>;
		using scale_timed_t	   = position_timed_t;

		bone_t() = default;
		bone_t(auto&& name) : m_Name(std::forward<decltype(name)>(name)) {}
		template <typename T>
		void name(T&& value) {
			m_Name = std::forward<T>(value);
		}

		psl::string8::view name() const noexcept { return m_Name; }

		void positions(psl::array<position_timed_t> values) { m_PositionData = std::move(values); }
		void rotations(psl::array<rotation_timed_t> values) { m_RotationData = std::move(values); }
		void scale(psl::array<scale_timed_t> values) { m_ScaleData = std::move(values); }

		inline void add_position(psl::vec3 value, precision_t time) { m_PositionData->emplace_back(value, time); }
		inline void add_rotation(psl::quat value, precision_t time) { m_RotationData->emplace_back(value, time); }
		inline void add_scale(psl::vec3 value, precision_t time) { m_ScaleData->emplace_back(value, time); }

		auto const& positions() const noexcept { return m_PositionData; }

		void sort() const noexcept {
			std::sort(m_PositionData->begin(), m_PositionData->end());
			std::sort(m_RotationData->begin(), m_RotationData->end());
			std::sort(m_ScaleData->begin(), m_ScaleData->end());
		}

	  private:
		template <typename S>
		void serialize(S& serializer) {
			if constexpr(psl::serialization::details::IsEncoder<S>) {
				sort();
			}
			serializer << m_Name << m_PositionData << m_RotationData << m_ScaleData;
			if constexpr(psl::serialization::details::IsDecoder<S>) {
				sort();
			}
		}

		psl::serialization::property<"NAME", psl::string8_t> m_Name {};
		mutable psl::serialization::property<"POSITION", psl::array<position_timed_t>> m_PositionData {};
		mutable psl::serialization::property<"ROTATION", psl::array<rotation_timed_t>> m_RotationData {};
		mutable psl::serialization::property<"SCALE", psl::array<scale_timed_t>> m_ScaleData {};
	};

	void name(psl::string8::view name) { m_Name = name; }
	psl::string8::view name() const noexcept { return m_Name; }

	void duration(precision_t duration) { m_Duration = duration; }
	precision_t duration() const noexcept { return m_Duration; }

	void frames_per_second(precision_t fps) { m_FPS = fps; }
	precision_t frames_per_second() const noexcept { return m_FPS; }

	void bones(auto&& value) { m_Bones = std::forward<decltype(value)>(value); }
	psl::array_view<bone_t> bones() const noexcept { return m_Bones; }

  private:
	template <typename S>
	void serialize(S& serializer) {
		serializer << m_Name << m_Duration << m_FPS << m_Bones;
	}

	psl::serialization::property<"NAME", psl::string8_t> m_Name {};
	psl::serialization::property<"DURATION", precision_t> m_Duration {};
	psl::serialization::property<"FPS", precision_t> m_FPS {};
	psl::serialization::property<"BONES", psl::array<bone_t>> m_Bones {};
};
}	 // namespace core::data
