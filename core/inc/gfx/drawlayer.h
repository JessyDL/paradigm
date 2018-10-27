#pragma once
#include "stdafx.h"

namespace core::gfx
{
	struct drawlayer
	{
	  public:
		drawlayer(const psl::string& name, uint32_t priority = 1000u) : name(name), priority(priority){};
		psl::string name;
		uint32_t priority{1000u};

		bool operator<(const drawlayer& other) const { return priority < other.priority; }
	};
} // namespace core::gfx

namespace std
{
	template <>
	struct less<core::gfx::drawlayer>
	{
		bool operator()(const core::gfx::drawlayer& lhs, const core::gfx::drawlayer& rhs) const { return lhs < rhs; }
	};
} // namespace std


namespace std
{
	template <>
	struct hash<core::gfx::drawlayer>
	{
		size_t operator()(const core::gfx::drawlayer& k) const
		{
			// Compute individual hash values for first, second and third
			// http://stackoverflow.com/a/1646913/126995
			size_t res = 17;
			res		   = res * 31 + hash<psl::string>()(k.name);
			res		   = res * 31 + hash<uint32_t>()(k.priority);
			return res;
		}
	};

	template <>
	struct hash<core::gfx::drawlayer*>
	{
		size_t operator()(const core::gfx::drawlayer* k) const
		{
			size_t res = 17;
			res		   = res * 31 + hash<psl::string>()(k->name);
			res		   = res * 31 + hash<uint32_t>()(k->priority);
			return res;
		}
	};
} // namespace std