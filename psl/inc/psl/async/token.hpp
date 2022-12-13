#pragma once
#include "barrier.hpp"
#include "psl/array.hpp"
#include "psl/view_ptr.hpp"
#include <cstdint>
#include <future>

namespace psl::async
{
class scheduler;
class token final
{
	friend class scheduler;
	token(size_t value, psl::view_ptr<scheduler> scheduler) noexcept : m_Token(value), m_Scheduler(scheduler) {};

  public:
	token() noexcept						 = default;
	~token()								 = default;
	token(const token& other)				 = default;
	token(token&& other) noexcept			 = default;
	token& operator=(const token& other)	 = default;
	token& operator=(token&& other) noexcept = default;

	operator size_t() const noexcept { return m_Token; }

	void after(const token& other) noexcept;
	void after(const psl::array<token>& others) noexcept;
	void before(const token& other) noexcept;
	void before(const psl::array<token>& others) noexcept;

	void barriers(const psl::array<barrier>& barriers);
	void barriers(psl::array<std::future<barrier>>&& barriers);
	void barriers(std::future<barrier>&& barrier);
	void barriers(const psl::array<std::shared_future<barrier>>& barriers);
	void barriers(std::shared_future<barrier>& barrier);
	void consecutive(psl::array<token> others);

  private:
	size_t m_Token {0};
	psl::view_ptr<scheduler> m_Scheduler {nullptr};
};


class pack
{
  public:
  private:
	size_t m_Id;
	psl::array<token> m_Tokens;
	psl::view_ptr<scheduler> m_Scheduler {nullptr};
};
}	 // namespace psl::async
