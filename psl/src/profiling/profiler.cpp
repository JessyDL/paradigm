#include "psl/profiling/profiler.h"
#include "psl/application_utils.h"
#include "psl/platform_utils.h"
#include "psl/string_utils.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#pragma comment(lib, "Dbghelp.lib")
#include <Dbghelp.h>
#endif

using namespace psl::profiling;

profiler::scoped_block::scoped_block(profiler& profiler) noexcept : prf(&profiler) {};
profiler::scoped_block::~scoped_block() noexcept
{
	if(prf != nullptr) prf->scope_end();
}

profiler::scoped_block::scoped_block(scoped_block&& other) : prf(other.prf) { other.prf = nullptr; }
profiler::scoped_block::scoped_block(volatile scoped_block&& other) : prf(other.prf) { other.prf = nullptr; }
void profiler::frame_info::push(const psl::string& name) noexcept
{
	auto it = m_NameMap.find(name);
	auto id = IDCounter;
	if(it == std::end(m_NameMap))
	{
		m_NameMap[name]	   = IDCounter;
		m_IDMap[IDCounter] = name;
		++IDCounter;
	}
	else
	{
		id = it->second;
	}
	m_Stack += 1;
	m_Scopes.emplace_back(id, m_Stack);
	m_Scopes[m_Scopes.size() - 1].duration = m_Timer.elapsed<std::chrono::microseconds>();
}

void profiler::frame_info::push(uint64_t name) noexcept
{
	m_Stack += 1;
	m_Scopes.emplace_back(name, m_Stack, true);
	m_Scopes[m_Scopes.size() - 1].duration = m_Timer.elapsed<std::chrono::microseconds>();
}

void profiler::frame_info::pop() noexcept
{
	auto it = std::find_if(std::reverse_iterator(std::end(m_Scopes)),
						   std::reverse_iterator(std::begin(m_Scopes)),
						   [this](const auto& sInfo) { return m_Stack == sInfo.depth; });

	it->duration = m_Timer.elapsed<std::chrono::microseconds>() - it->duration;
	m_Stack -= 1;
}
void profiler::frame_info::clear()
{
	m_Scopes.clear();
	m_Timer.reset();
	m_NameMap.clear();
	m_IDMap.clear();
	IDCounter = 0;
	m_Stack	  = 0;
}
void profiler::frame_info::end()
{
#ifdef PE_PROFILER
	duration = m_Timer.elapsed<std::chrono::microseconds>();
#endif
}

profiler::profiler(size_t buffer_size)
{
#ifdef PE_PROFILER
	m_Frames.resize(buffer_size);
#endif
}

void profiler::next_frame()
{
#ifdef PE_PROFILER
	m_Frames[m_FrameIndex].end();
	m_FrameIndex = (m_FrameIndex + 1) % m_Frames.size();
	m_Frames[m_FrameIndex].clear();
#endif
}


volatile profiler::scoped_block profiler::scope(const psl::string& name) noexcept
{
#ifdef PE_PROFILER
	m_Frames[m_FrameIndex].push(name);
#endif
	return {*this};
}
volatile profiler::scoped_block profiler::scope(const psl::string& name, void* target) noexcept
{
#ifdef PE_PROFILER
	m_Frames[m_FrameIndex].push(name);
#endif
	return {*this};
}

volatile profiler::scoped_block profiler::scope(void* target) noexcept
{
#ifdef PE_PROFILER
	m_Frames[m_FrameIndex].push((std::uintptr_t)target);
#endif
	return {*this};
}

volatile profiler::scoped_block profiler::scope() noexcept
{
#ifdef PE_PROFILER
	auto res = utility::debug::raw_trace(1, 1);
	m_Frames[m_FrameIndex].push((std::uintptr_t)res[0]);
#endif
	return {*this};
}

void profiler::scope_begin(const psl::string& name)
{
#ifdef PE_PROFILER
	m_Frames[m_FrameIndex].push(name);
#endif
}
void profiler::scope_begin(const psl::string& name, void* target)
{
#ifdef PE_PROFILER
	m_Frames[m_FrameIndex].push(name);
#endif
}

void profiler::scope_end()
{
#ifdef PE_PROFILER
	m_Frames[m_FrameIndex].pop();
#endif
}
void profiler::scope_end(void* target)
{
#ifdef PE_PROFILER
	m_Frames[m_FrameIndex].pop();
#endif
}


psl::string profiler::to_string() const
{
	psl::string res;
#ifdef PE_PROFILER
	psl::bytell_map<uint64_t, psl::string> demangled_info;
	const auto endIt = (m_FrameIndex + 1) % m_Frames.size();
	auto i			 = endIt;
#ifdef PLATFORM_WINDOWS
	/*auto mapping = utility::platform::file::read(utility::application::path::get_path() + "core.map");
	if(mapping)
	{
		auto index = mapping.value().find_first_not_of("\r\n\t ", mapping.value().find("Lib:Object") + 10);
		bool reached_end = false;
		psl::string demangled;
		demangled.resize(256);
		while(!reached_end)
		{
			auto end = mapping.value().find('\n', index);
			psl::string line = mapping.value().substr(index, end - index);
			auto elements = utility::string::split(line, " ", true);
			psl::string mangled{elements[1]};
			auto size = UnDecorateSymbolName(mangled.c_str(), demangled.data(), 256, UNDNAME_COMPLETE);
			demangled_info[psl::string(elements[1])] = demangled.substr(0, size);
			index = end + 1;
			if(mapping.value()[index] == '\r' || mapping.value()[index] == '\n')
				reached_end = true;
		}
	}*/
#endif
	do
	{
		const auto& frame_data = m_Frames[i];
		for(const auto& scope : frame_data.m_Scopes)
		{
			if(scope.mangled_name)
			{
				if(auto it = demangled_info.find(scope.name); it == std::end(demangled_info))
				{
					demangled_info.insert({scope.name, utility::debug::demangle((void*)scope.name).name});
				}
			}
		}
		i = (i + 1) % m_Frames.size();
	} while(i != endIt);
	psl::timer init_timer;
	i = endIt;
	do
	{
		const auto& frame_data = m_Frames[i];
		std::chrono::microseconds duration =
		  (frame_data.duration.count() == 0)
			? frame_data.m_Timer.elapsed<std::chrono::microseconds>() - init_timer.elapsed<std::chrono::microseconds>()
			: frame_data.duration;
		res += "--------------------------------------------------------------------------------\nframe\n";
		res += "\t duration: " + utility::converter<decltype(duration.count())>::to_string(duration.count()) + u8"μs\n";
		res += "\t invocations: " + utility::converter<size_t>::to_string(frame_data.m_Scopes.size()) + "\n";
		for(const auto& scope : frame_data.m_Scopes)
		{
			double percentage		  = (double)(scope.duration.count() * 1000000 / duration.count()) / 10000.0;
			psl::string percentageStr = utility::converter<double>::to_string(percentage);

			if(percentage < 10.0) percentageStr.insert(std::begin(percentageStr), '0');
			percentageStr.resize(percentageStr.size() - 2);
			percentageStr += "%";
			psl::string durationStr = utility::converter<size_t>::to_string(scope.duration.count()) + u8"μs";
			psl::string name;
			if(scope.mangled_name)
			{
				if(auto it = demangled_info.find(scope.name); it != std::end(demangled_info))
				{
					name = it->second;
				}
			}
			else
			{
				name = frame_data.m_IDMap.at(scope.name);
			}
			size_t bufferSize = scope.depth * 2 + 20 - durationStr.size() - percentageStr.size();
			res += psl::string(scope.depth * 2, ' ') + percentageStr + " - " + durationStr +
				   psl::string(std::max(bufferSize, (size_t)2), ' ') + name + "\n";
		}
		res += "endframe\n--------------------------------------------------------------------------------\n";
		i = (i + 1) % m_Frames.size();
	} while(i != endIt);
#endif
	return res;
}