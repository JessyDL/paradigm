#include "stdafx_psl.h"
#include "task.h"
using namespace psl::async;


bool barriers::memory::is_dependent_on(const barriers::memory& other) const noexcept
{
	for (auto& dep : m_Barriers)
	{
		for (auto& read_dep : dep)
		{
			for (auto& other_write_dep : other.m_Barriers[1])
			{
				if (read_dep.overlaps(other_write_dep))
					return true;
			}
		}
	}
	return false;
}

void barriers::memory::add(const ::memory::range& range, barrier barrier)
{
	size_t dep = (size_t)barrier;
	m_Barriers[dep].insert(std::upper_bound(std::begin(m_Barriers[dep]), std::end(m_Barriers[dep]), range), range);
	//m_Barriers[dep].push_back(range);
}


void barriers::memory::remove(const ::memory::range& range, barrier barrier)
{
	size_t dep = (size_t)barrier;
	auto it = std::find(std::begin(m_Barriers[dep]), std::end(m_Barriers[dep]), range);
	if (it != std::end(m_Barriers[dep]))
		m_Barriers[dep].erase(it);
}

void barriers::memory::clear()
{
	for (auto& it : m_Barriers)
	{
		it.clear();
	}
}

bool barriers::memory::can_be_concurrent(const barriers::memory& otherMem) const noexcept
{
	return
		!is_dependent_on(otherMem) &&
		!otherMem.is_dependent_on(*this);
}

void barriers::task::add(details::task_token* task)
{
	m_TaskBarrier.push_back(task);
}

void barriers::task::remove(details::task_token* task)
{
	m_TaskBarrier.erase(std::find(std::begin(m_TaskBarrier), std::end(m_TaskBarrier), task));
}
void barriers::task::clear()
{
	m_TaskBarrier.clear();
}


bool barriers::task::contains(details::task_token* task) const noexcept
{
	return std::find(std::begin(m_TaskBarrier), std::end(m_TaskBarrier), task) != std::end(m_TaskBarrier);
}

bool barriers::task::can_be_concurrent(const barriers::task& otherTask) const noexcept
{
	return
		!is_dependent_on(otherTask) &&
		!otherTask.is_dependent_on(*this);
}
bool barriers::task::is_dependent_on(const barriers::task & other) const noexcept
{
	return std::find(std::begin(m_TaskBarrier), std::end(m_TaskBarrier), other.m_Task) == std::end(m_TaskBarrier);
}
