#pragma once
#include "array_view.hpp"
#include "meta.hpp"
#include "serialization.hpp"
#include "vector.hpp"

namespace core::resource {
class cache_t;
}

namespace core::data {
class bundle final {
	friend class psl::serialization::accessor;
	template <typename T, char... Char>
	using sproperty = psl::serialization::property<T, Char...>;

	class data final {
		friend class psl::serialization::accessor;

	  public:
		data() noexcept = default;
		data(const psl::UID& material, uint32_t layer) noexcept;

		~data() = default;

		data(const data&)				 = default;
		data(data&&) noexcept			 = default;
		data& operator=(const data&)	 = default;
		data& operator=(data&&) noexcept = default;

		const psl::UID& material() const noexcept;
		void material(const psl::UID& value) noexcept;

		uint32_t layer() const noexcept;
		void layer(uint32_t value) noexcept;

	  private:
		/// \brief serialization method to be used by the serializer when writing this container to the disk.
		/// \param[in] serializer the serialization object, consult the serialization namespace for more
		/// information.
		template <typename S>
		void serialize(S& serializer) {
			serializer << m_Material << m_Layer;
		};
		sproperty<psl::UID, const_str("MATERIAL", 8)> m_Material;
		sproperty<uint32_t, const_str("LAYER", 5)> m_Layer;
	};

  public:
	bundle() noexcept = default;
	bundle(const psl::UID& uid, core::resource::cache_t& cache) noexcept;

	~bundle() = default;

	bundle(const bundle&)				 = default;
	bundle(bundle&&) noexcept			 = default;
	bundle& operator=(const bundle&)	 = default;
	bundle& operator=(bundle&&) noexcept = default;

	psl::array_view<data> materials() const noexcept;
	void add(psl::array_view<std::pair<psl::UID, uint32_t>> materials);
	void remove(psl::UID material) noexcept;
	void remove(psl::UID material, uint32_t layer) noexcept;
	void remove(uint32_t layer) noexcept;

  private:
	/// \brief serialization method to be used by the serializer when writing this container to the disk.
	/// \param[in] serializer the serialization object, consult the serialization namespace for more information.
	template <typename S>
	void serialize(S& serializer) {
		serializer << m_Data;
	};

	sproperty<psl::array<data>, const_str("DATA", 4)> m_Data;
};
}	 // namespace core::data
