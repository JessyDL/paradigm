#include "stdafx_psl.h"
#include "profiling/profiler.h"
#include "string_utils.h"

using namespace psl::profiling;

profiler::scoped_block::scoped_block(profiler& profiler) noexcept : prf(&profiler) {};
profiler::scoped_block::~scoped_block() noexcept
{
	if(prf != nullptr)
		prf->scope_end();
}

profiler::scoped_block::scoped_block(scoped_block&& other) : prf(other.prf){other.prf = nullptr;}

void profiler::frame_info::push(const psl::string& name) noexcept
{
	auto it = m_NameMap.find(name);
	auto id = IDCounter;
	if(it == std::end(m_NameMap))
	{
		m_NameMap[name] = IDCounter;
		m_IDMap[IDCounter] = name;
		++IDCounter;
	}
	else
	{
		id = it->second;
	}
	m_Stack+=1;
	m_Scopes.emplace_back(id, m_Stack);
	m_Scopes[m_Scopes.size() -1].duration = m_Timer.elapsed<std::chrono::microseconds>();
}

void profiler::frame_info::pop() noexcept
{
	auto it = std::find_if(std::reverse_iterator(std::end(m_Scopes)),
						   std::reverse_iterator(std::begin(m_Scopes)),
						   [this](const auto& sInfo) { return m_Stack == sInfo.depth; });

	it->duration = m_Timer.elapsed<std::chrono::microseconds>() - it->duration;
	m_Stack-=1;
}
void profiler::frame_info::clear()
{
	m_Scopes.clear();
	m_Timer.reset();
	m_NameMap.clear();
	m_IDMap.clear();
	IDCounter = 0;
	m_Stack = 0;
}
void profiler::frame_info::end()
{
	duration = m_Timer.elapsed<std::chrono::microseconds>();
}
void profiler::next_frame()
{
	m_Frames[m_FrameIndex].end();
	m_FrameIndex = (m_FrameIndex + 1) % m_Frames.size();
	m_Frames[m_FrameIndex].clear();
}

profiler::profiler(size_t buffer_size)
{
	m_Frames.resize(buffer_size);
}

volatile profiler::scoped_block profiler::scope(const psl::string& name) noexcept
{
	m_Frames[m_FrameIndex].push(name);
	return {*this};
}

void profiler::scope_begin(const psl::string& name)
{
	m_Frames[m_FrameIndex].push(name);
}

void profiler::scope_end()
{
	m_Frames[m_FrameIndex].pop();
}


psl::string profiler::to_string() const
{
	psl::timer init_timer;
	psl::string res;
	const auto endIt = (m_FrameIndex + 1) % m_Frames.size();
	auto i = endIt;
	do
	{
		const auto& frame_data = m_Frames[i];
		std::chrono::microseconds duration = (frame_data.duration.count() == 0) ? frame_data.m_Timer.elapsed<std::chrono::microseconds>() - init_timer.elapsed< std::chrono::microseconds>() : frame_data.duration;
		res += "--------------------------------------------------------------------------------\nframe\n";
		res += "\t duration: " + utility::converter<long long>::to_string(duration.count()) + u8"μs\n";
		res += "\t invocations: " + utility::converter<long long>::to_string(frame_data.m_Scopes.size()) + "\n";
		for(const auto& scope : frame_data.m_Scopes)
		{
			double percentage = (double)(scope.duration.count() * 1000000 / duration.count()) / 10000.0;
			psl::string percentageStr = utility::converter<double>::to_string(percentage);

			if(percentage < 10.0)
				percentageStr.insert(std::begin(percentageStr), '0');
			percentageStr.resize(percentageStr.size() - 2);
			percentageStr += "%";
			psl::string durationStr = utility::converter<long long>::to_string(scope.duration.count()) + u8"μs";

			int32_t bufferSize = scope.depth * 2 + 20 - durationStr.size() - percentageStr.size();
			res += psl::string(scope.depth * 2, ' ') + percentageStr + " - " + durationStr + psl::string(std::max(bufferSize, 2), ' ') + frame_data.m_IDMap.at(scope.name) + "\n";
		}
		res += "endframe\n--------------------------------------------------------------------------------\n";
		i = (i + 1) % m_Frames.size();
	}
	while(i != endIt);
	return res;
}