#pragma once
#include <cstdint>
#include <future>
#include "view_ptr.h"
#include "array.h"
#include "barrier.h"

namespace psl::async
{
	class scheduler;
	class token final
	{
		friend class scheduler;
		token(size_t value, psl::view_ptr<scheduler> scheduler) noexcept : m_Token(value), m_Scheduler(scheduler){};

	  public:
		token() noexcept = default;
		~token()		 = default;
		token(const token& other) : m_Token(other.m_Token), m_Scheduler(other.m_Scheduler){};
		token(token&& other) noexcept : m_Token(other.m_Token), m_Scheduler(std::move(other.m_Scheduler)){};
		token& operator=(const token& other)
		{
			if(this != &other)
			{
				m_Token		= other.m_Token;
				m_Scheduler = other.m_Scheduler;
			}
			return *this;
		}
		token& operator=(token&& other) noexcept
		{
			if(this != &other)
			{
				m_Token		= other.m_Token;
				m_Scheduler = std::move(other.m_Scheduler);
			}
			return *this;
		}

		operator size_t() const noexcept { return m_Token; }

		void after(token other) noexcept;
		void after(psl::array<token> others) noexcept;
		void before(token other) noexcept;
		void before(psl::array<token> others) noexcept;

		void barriers(const psl::array<barrier>& barriers);
		void barriers(psl::array<std::future<barrier>>&& barriers);
		void barriers(const psl::array<std::shared_future<barrier>>& barriers);
		void consecutive(psl::array<token> others);

	  private:
		size_t m_Token{0};
		psl::view_ptr<scheduler> m_Scheduler{nullptr};
	};


	class pack
	{
	  public:
	  private:
		size_t m_Id;
		psl::array<token> m_Tokens;
		psl::view_ptr<scheduler> m_Scheduler{nullptr};
	};
} // namespace psl::async