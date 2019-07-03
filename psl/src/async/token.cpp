#include "async/token.h"
#include "async/scheduler.h"

using namespace psl::async2;

void token::after(token other) noexcept { m_Scheduler->sequence(other, *this); }
void token::after(psl::array<token> others) noexcept { m_Scheduler->sequence(others, *this); }
void token::before(token other) noexcept { m_Scheduler->sequence(*this, other); }
void token::before(psl::array<token> others) noexcept { m_Scheduler->sequence(*this, others); }

void token::barriers(psl::array<barrier> barriers) noexcept { m_Scheduler->barriers(*this, barriers); }
void token::consecutive(psl::array<token> others) noexcept { m_Scheduler->consecutive(*this, others); }
