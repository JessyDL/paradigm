#include "resource/resource.hpp"
#include "profiling/profiler.h"
#include "logging.h"
#include "assertions.h"
#include <cassert>

using namespace core::resource;


cache::cache(psl::meta::library&& library, memory::allocator_base* allocator)
	: m_Library(std::move(library)), m_Allocator(allocator)
{
	PROFILE_SCOPE(core::profiler)
	assert(m_Allocator != nullptr && m_Allocator->is_physically_backed());
	LOG_INFO("creating cache");
}

cache::~cache()
{
	PROFILE_SCOPE(core::profiler)
	LOG_INFO("destroying cache start");
	bool bErased	 = false;
	size_t iteration = 0u;
	bool bLeaks		 = false;
	do
	{
		bErased		 = false;
		bLeaks		 = false;
		size_t count = 0u;
		for(auto& pair : m_Handles)
		{
			for(auto& it : pair.second)
			{
				bLeaks |= it.state == state::LOADED;
				if(it.container.use_count() == 1 && it.state == state::LOADED)
				{
					it.state = state::UNLOADING;
					it.m_Table.clear(it.container.get());
					it.state = state::UNLOADED;
					bErased  = true;
					++count;
				}
			}
		}
		LOG_INFO("iteration ", utility::to_string(iteration++), " destroyed ", utility::to_string(count), " objects");
	} while(bErased); // keep looping as long as items are present


	if(bLeaks)
	{
		LOG_WARNING("leaks detected!");

		for(auto& pair : m_Handles)
		{
			for(auto& it : pair.second)
			{
				if(it.state == state::LOADED)
				{
#ifdef DEBUG_CORE_RESOURCE

					core::log->warn("\ttype {0} uid {1}", it.m_Table.type_name(it.container.get()),
									utility::to_string(pair.first));
#else
					core::log->warn("\ttype {0} uid {1}", utility::to_string((std::uintptr_t)(it.id)),
									utility::to_string(pair.first));
#endif
				}
			}
		}
	}
	core::log->flush();

	LOG_INFO("destroying cache end");
}

void cache::free(bool recursive)
{
	PROFILE_SCOPE(core::profiler)
	bool bErased	 = false;
	size_t iteration = 0u;
	do
	{
		bErased		 = false;
		size_t count = 0u;
		for(auto& pair : m_Handles)
		{
			for(auto& it : pair.second)
			{
				if(it.container.use_count() == 1 && it.state == state::LOADED)
				{
					it.state = state::UNLOADING;
					it.m_Table.clear(it.container.get());
					it.state = state::UNLOADED;
					bErased  = true;
					++count;
				}
			}
		}
		LOG_INFO("iteration ", utility::to_string(iteration++), " destroyed ", utility::to_string(count), " objects");
	} while(bErased && recursive); // keep looping as long as items are present
}