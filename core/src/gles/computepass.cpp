#include "gles/computepass.h"
#include "gles/igles.h"

using namespace core::igles;
using namespace core::resource;


void computepass::clear() {}
void computepass::prepare() {}
bool computepass::build() { return true; }
void computepass::present()
{
	for(auto& barrier : m_MemoryBarriers)
	{
		glMemoryBarrier(barrier.barrier);
	}

	for(auto& compute : m_Compute)
	{
		compute.dispatch();
	}
}

void computepass::add(psl::array_view<core::gfx::computecall> compute)
{
	m_Compute.insert(std::end(m_Compute), std::begin(compute), std::end(compute));
}
void computepass::add(const core::gfx::computecall& compute) { m_Compute.emplace_back(compute); }

psl::array<GLbitfield> computepass::memory_barriers() const noexcept
{
	// todo: this is currently hard coded
	core::igles::log->debug("todo: need to implement memory barriers for compute");
	return {GL_SHADER_IMAGE_ACCESS_BARRIER_BIT};
}

void computepass::connect(psl::view_ptr<drawpass> pass) noexcept {};
void computepass::disconnect(psl::view_ptr<drawpass> pass) noexcept {};

void computepass::connect(psl::view_ptr<computepass> pass) noexcept
{
	for(const auto& barrier : pass->memory_barriers())
	{
		if(auto it = std::find_if(std::begin(m_MemoryBarriers),
								  std::end(m_MemoryBarriers),
								  [&barrier](auto&& item) { return barrier == item.barrier; });
		   it != std::end(m_MemoryBarriers))
			it->usage += 1;
		else
			m_MemoryBarriers.emplace_back(computepass::memory_barrier_t {barrier, 1});
	}
};
void computepass::disconnect(psl::view_ptr<computepass> pass) noexcept
{
	for(const auto& barrier : pass->memory_barriers())
	{
		if(auto it = std::find_if(std::begin(m_MemoryBarriers),
								  std::end(m_MemoryBarriers),
								  [&barrier](auto&& item) { return barrier == item.barrier; });
		   it != std::end(m_MemoryBarriers))
		{
			it->usage -= 1;
			if(it->usage == 0) m_MemoryBarriers.erase(it);
		}
	}
};