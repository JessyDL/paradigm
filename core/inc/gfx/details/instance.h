#pragma once
#include "vector.h"
#include "systems/resource.h"
#include "gfx/material.h"
#include "meta.h"
#include "sparse_array.h"

namespace std
{
#ifdef _MSC_VER
	template <typename T>
	struct hash;
#endif
} // namespace std

namespace core::gfx::details::instance
{
	struct binding
	{
		bool operator==(const binding& b) const noexcept {
			return size_of_element == b.size_of_element && name == b.name;
		}
		psl::string name{};
		uint32_t size_of_element{0};
	};

	struct object final
	{
		object(psl::UID uid) : geometry(uid), id_generator(0){};
		object(psl::UID uid, uint32_t capacity) : geometry(uid), id_generator(capacity){};

		bool operator==(const object& rhs) const noexcept { return rhs.geometry == geometry; }

		const psl::UID geometry; // Defines which geometry this object maps to
		psl::generator<uint32_t> id_generator;
		psl::array<memory::segment> data;
	};
} // namespace core::gfx::details::instance


namespace std
{
	template <>
	struct hash<core::gfx::details::instance::object>
	{
		std::size_t operator()(const core::gfx::details::instance::object& s) const noexcept
		{
			return std::hash<psl::UID>{}(s.geometry);
		}
		std::size_t operator()(const psl::UID& s) const noexcept { return std::hash<psl::UID>{}(s); }
	};

	template <>
	struct hash<core::gfx::details::instance::binding>
	{
		std::size_t operator()(const core::gfx::details::instance::binding& s) const noexcept
		{
			std::size_t seed = std::hash<psl::string>{}(s.name);
			seed ^= (uint64_t)s.size_of_element + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
} // namespace std


namespace core::gfx::details::instance
{
	class data final
	{
	  public:
		data(core::resource::handle<core::gfx::buffer> buffer) noexcept : m_InstanceBuffer(buffer){};
		void add(core::resource::handle<material> material);
		uint32_t add(core::resource::tag<core::gfx::geometry> uid);

		void remove(core::resource::handle<material> material);

		bool has_element(psl::string_view name) const noexcept { return false; };
		bool has_element(uint32_t binding) const noexcept { return false; };

		uint32_t count(core::resource::tag<core::gfx::geometry> uid) const noexcept;

	  private:
		std::unordered_map<psl::UID, psl::array<binding>> m_Bindings;
		psl::array<std::pair<binding, uint32_t>> m_UniqueBindings; // unique binding and usage count
		std::unordered_map<psl::UID, object> m_InstanceData;

		core::resource::handle<core::gfx::buffer> m_InstanceBuffer;
	};
} // namespace core::gfx::details::instance
