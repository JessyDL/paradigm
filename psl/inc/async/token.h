#pragma once
#include <cstdint>
#include "view_ptr.h"
#include "array.h"
#include "barrier.h"

namespace psl::async2
{
	class scheduler;
	class token final
	{
		friend class scheduler;
		token(size_t value, psl::view_ptr<scheduler> scheduler) noexcept : m_Token(value), m_Scheduler(scheduler){};

	  public:
		token() noexcept				 = default;
		~token()						 = default;
		token(const token& tok) noexcept = default;
		token(token&& tok) noexcept		 = default;
		token& operator=(const token& tok) noexcept = default;
		token& operator=(token&& tok) noexcept = default;

		operator size_t() const noexcept { return m_Token; }

		void after(token other) noexcept;
		void after(psl::array<token> others) noexcept;
		void before(token other) noexcept;
		void before(psl::array<token> others) noexcept;

		void barriers(psl::array<barrier> barriers) noexcept;
		void consecutive(psl::array<token> others) noexcept;

	  private:
		size_t m_Token{0};
		psl::view_ptr<scheduler> m_Scheduler{nullptr};
	};
} // namespace psl::async2