#include "psl/async/token.h"
#include "psl/async/scheduler.h"

using namespace psl::async;

void token::after(const token& other) noexcept { m_Scheduler->sequence(other, *this); }
void token::after(const psl::array<token>& others) noexcept { m_Scheduler->sequence(others, *this); }
void token::before(const token& other) noexcept { m_Scheduler->sequence(*this, other); }
void token::before(const psl::array<token>& others) noexcept { m_Scheduler->sequence(*this, others); }

void token::barriers(const psl::array<barrier>& barriers) { m_Scheduler->barriers(*this, barriers); }
void token::barriers(psl::array<std::future<barrier>>&& barriers) { m_Scheduler->barriers(*this, std::move(barriers)); }
void token::barriers(std::future<barrier>&& barrier) { m_Scheduler->barriers(*this, std::move(barrier)); }
void token::barriers(const psl::array<std::shared_future<barrier>>& barriers)
{
	m_Scheduler->barriers(*this, barriers);
}
void token::barriers(std::shared_future<barrier>& barrier) { m_Scheduler->barriers(*this, barrier); }
void token::consecutive(psl::array<token> others) { m_Scheduler->consecutive(*this, others); }
