#pragma once
#include "details/packet.hpp"
#include "psl/array.hpp"
#include "psl/collections/spmc/producer.hpp"
#include "psl/template_utils.hpp"
#include "psl/unique_ptr.hpp"
#include "psl/ustring.hpp"
#include "token.hpp"
#include <future>
#include <optional>

namespace psl::async::details {
struct worker;
}
namespace psl::async {
class scheduler final {
  public:
	scheduler(std::optional<size_t> workers = std::nullopt, psl::string_view name = "") noexcept;
	~scheduler();

	template <template <typename> typename Future = std::future, typename Fn>
		requires(std::is_same_v<decltype(std::declval<Fn>()()), psl::async::result>)
	auto schedule(Fn&& func, std::optional<size_t> max_retries = std::nullopt) -> token {
		using return_t = decltype(std::declval<Fn>()());
		auto token	   = proxy(max_retries);
		substitute<Future, Fn>(token, std::forward<decltype(func)>(func));
		return token;
	}

	template <template <typename> typename Future = std::future, typename Fn>
		requires(!std::is_same_v<decltype(std::declval<Fn>()()), psl::async::result>)
	auto schedule(Fn&& func) -> std::conditional_t<std::is_same_v<decltype(std::declval<Fn>()()), void>,
												   token,
												   std::pair<token, Future<decltype(std::declval<Fn>()())>>> {
		using return_t = decltype(std::declval<Fn>()());
		auto token	   = proxy();

		if constexpr(std::is_same_v<void, return_t>) {
			substitute<Future, Fn>(token, std::forward<decltype(func)>(func));
			return token;
		} else {
			return std::pair {token, substitute<Future, Fn>(token, std::forward<decltype(func)>(func))};
		}
	}

  private:
	token proxy(std::optional<size_t> max_retries = std::nullopt) {
		auto token {async::token {m_Invocables.size() + m_TokenOffset, psl::view_ptr<scheduler> {this}}};
		m_Invocables.emplace_back(token, max_retries);
		return token;
	}


	template <template <typename> typename Future = std::future, typename Fn>
	auto substitute(token& token, Fn&& func)
	  -> std::conditional_t<std::is_same_v<decltype(std::declval<Fn>()()), void> ||
							  std::is_same_v<decltype(std::declval<Fn>()()), psl::async::result>,
							void,
							Future<decltype(std::declval<Fn>()())>> {
		using return_t	= decltype(std::declval<Fn>()());
		using storage_t = typename std::
		  conditional<std::is_same<decltype(std::declval<Fn>()()), void>::value, void, Future<return_t>>::type;

		auto task = new details::task<return_t, Fn, storage_t>(std::forward<decltype(func)>(func));
		m_Invocables[token - m_TokenOffset].substitute(task);

		if constexpr(!std::is_same_v<void, return_t> && !std::is_same_v<return_t, psl::async::result>) {
			return task->future();
		}
	}

  public:
	void execute();

	void sequence(token first, token then) noexcept;
	void sequence(psl::array<token> first, token then) noexcept;
	void sequence(token first, psl::array<token> then) noexcept;
	void sequence(psl::array<token> first, psl::array<token> then) noexcept;

	void barriers(token token, const psl::array<barrier>& barriers);
	void barriers(token token, psl::array<std::future<barrier>>&& barriers);
	void barriers(token token, std::future<barrier>&& barrier);
	void barriers(token token, const psl::array<std::shared_future<barrier>>& barriers);
	void barriers(token token, std::shared_future<barrier>& barrier);
	void consecutive(token target, psl::array<token> tokens);

	size_t workers() const noexcept {
		return m_Workers;
	};

  private:
	size_t m_Workers {4};
	size_t m_TokenOffset {0u};
	psl::array<details::packet> m_Invocables;
	psl::array<psl::unique_ptr<details::worker>> m_Workerthreads;
	psl::spmc::producer<psl::view_ptr<details::packet>> m_Tasks;
};
}	 // namespace psl::async
