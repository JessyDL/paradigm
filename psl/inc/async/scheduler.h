#pragma once
#include <future>
#include <optional>
#include "token.h"
#include "template_utils.h"
#include "array.h"
#include "unique_ptr.h"
#include "collections/spmc/producer.h"
#include "details/packet.h"

namespace psl::async::details
{
	struct worker;
}
namespace psl::async
{
	class scheduler final
	{
	  public:
		scheduler(std::optional<size_t> workers = std::nullopt) noexcept;
		~scheduler();
		template <template <typename> typename Future = std::future, typename Fn>
		auto schedule(Fn&& func) -> std::pair<token, Future<decltype(func())>>
		{
			using return_t = decltype(func());

			auto tok{token{m_Invocables.size() + m_TokenOffset, psl::view_ptr<scheduler>{this}}};

			auto lambda = [func = std::forward<Fn>(func)](std::promise<return_t>& promise) mutable {
				if constexpr(std::is_same<return_t, void>::value)
				{
					std::invoke(func);
					promise.set_value();
				}
				else
				{
					promise.set_value(std::move(std::invoke(func)));
				}
			};

			auto task = new details::task<return_t, decltype(lambda), Future<return_t>>(std::move(lambda));
			m_Invocables.emplace_back(tok, psl::unique_ptr<details::task_base>{task});
			return std::pair{tok, task->future()};
		}

		void execute();

		void sequence(token first, token then) noexcept;
		void sequence(psl::array<token> first, token then) noexcept;
		void sequence(token first, psl::array<token> then) noexcept;
		void sequence(psl::array<token> first, psl::array<token> then) noexcept;

		void barriers(token token, const psl::array<barrier>& barriers);
		void barriers(token token, psl::array<std::future<barrier>>&& barriers);
		void barriers(token token, const psl::array<std::shared_future<barrier>>& barriers);
		void consecutive(token target, psl::array<token> tokens);

		size_t workers() const noexcept { return m_Workers; };

	  private:
		size_t m_Workers{4};
		size_t m_TokenOffset{0u};
		psl::array<details::packet> m_Invocables;
		psl::array<psl::unique_ptr<details::worker>> m_Workerthreads;
		psl::spmc::producer<psl::view_ptr<details::packet>> m_Tasks;
	};
} // namespace psl::async2