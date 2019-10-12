#include "gfx/computecall.h"
#include "gfx/compute.h"

using namespace core::gfx;

computecall::computecall(core::resource::handle<compute> compute) : m_Compute(compute) {}

void computecall::dispatch() { m_Compute->dispatch(m_DispatchSize); }