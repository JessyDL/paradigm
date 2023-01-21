#include "core/gfx/computecall.hpp"
#include "core/gfx/compute.hpp"

using namespace core::gfx;

computecall::computecall(core::resource::handle<compute> compute) : m_Compute(compute) {}

void computecall::dispatch() {
	m_Compute->dispatch(m_DispatchSize);
}
