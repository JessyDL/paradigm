#include "core/vk/computepass.hpp"
#include "core/gfx/computecall.hpp"
#include "core/gfx/drawpass.hpp"

using namespace core::ivk;
using computecall = core::gfx::computecall;

void computepass::clear() {}
void computepass::prepare() {}
bool computepass::build() {
	return true;
}
void computepass::present() {
	for(auto& compute : m_Compute) {
		compute.dispatch();
	}
}

void computepass::add(psl::array_view<computecall> compute) {
	m_Compute.insert(std::end(m_Compute), std::begin(compute), std::end(compute));
}
void computepass::add(const computecall& compute) {
	m_Compute.emplace_back(compute);
}

void computepass::connect(psl::view_ptr<drawpass> pass) noexcept {}
void computepass::connect(psl::view_ptr<computepass> pass) noexcept {}
void computepass::disconnect(psl::view_ptr<drawpass> pass) noexcept {}
void computepass::disconnect(psl::view_ptr<computepass> pass) noexcept {}
