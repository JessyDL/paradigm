#include "systems/resource.h"
#include "profiling/profiler.h"
#include "logging.h"

using namespace core::resource;

uint64_t cache::id{0};


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
	bool bErased = false;
	size_t iteration = 0u;
	size_t count = 0u;
	bool bLeaks = false;
	do
	{
		bErased = false;
		bLeaks = false;
		count = 0u;
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
					bErased = true;
					++count;
				}
			}
		}
		LOG_INFO("iteration ", utility::to_string(iteration++), " destroyed ", utility::to_string(count),
				 " objects");
	} while(bErased); // keep looping as long as items are present


	if(bLeaks)
	{
		LOG_WARNING("leaks detected!");
	}

	LOG_INFO("destroying cache end");
}

void cache::free(bool recursive)
{
	PROFILE_SCOPE(core::profiler)
		bool bErased = false;
	size_t iteration = 0u;
	size_t count = 0u;
	do
	{
		bErased = false;
		count = 0u;
		for(auto& pair : m_Handles)
		{
			for(auto& it : pair.second)
			{
				if(it.container.use_count() == 1 && it.state == state::LOADED)
				{
					it.state = state::UNLOADING;
					it.m_Table.clear(it.container.get());
					it.state = state::UNLOADED;
					bErased = true;
					++count;
				}
			}
		}
		LOG_INFO("iteration ", utility::to_string(iteration++), " destroyed ", utility::to_string(count),
				 " objects");
	} while(bErased && recursive); // keep looping as long as items are present
}