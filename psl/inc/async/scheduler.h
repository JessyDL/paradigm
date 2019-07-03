#pragma once
#include <future>
#include <optional>
#include "token.h"
#include "template_utils.h"
#include "array.h"
#include "unique_ptr.h"
#include "collections/spmc.h"
#include "details/packet.h"

namespace psl::collections
{
	template <typename T>
	class graph
	{
	  public:
		class node
		{
			T* m_Item;
			psl::array<psl::view_ptr<node>> m_Next;
			size_t m_Depth{0};
		};

		class strand
		{
			psl::array<node*> m_Nodes;
		};

		class branch
		{
			strand m_Strand;
			psl::array<strand*> m_Next;
		};

		node& push_back(T&& item) {}

	  private:
		psl::array<node*> m_Nodes;
	};
} // namespace psl::collections

#define FWD(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace impl
{

	template <typename... Ts>
	auto fwd_capture(Ts&&... xs)
	{
		return std::tuple<Ts...>(FWD(xs)...);
	}

} // namespace impl

template <typename T>
decltype(auto) access(T&& x)
{
	return std::get<0>(FWD(x));
}

#define FWD_CAPTURE(...) impl::fwd_capture(FWD(__VA_ARGS__))

namespace impl
{

	template <typename... Ts>
	auto fwd_capture_as_tuple(Ts&&... xs)
	{
		return std::tuple<Ts...>(std::forward<Ts>(xs)...);
	}

} // namespace impl

#define FWD_CAPTURE_PACK(...) impl::fwd_capture_as_tuple(FWD(__VA_ARGS__)...)

// Expand all elements of `fc` into a `f(...)` invocation.
template <typename TF, typename TFwdCapture>
decltype(auto) apply_fwd_capture_pack(TF&& f, TFwdCapture&& fc)
{
	return std::apply([&f](auto&&... xs) -> decltype(auto) { return f(access(FWD(xs))...); }, FWD(fc));
}

namespace psl::async2
{
	class scheduler final
	{
	  public:
		scheduler(std::optional<size_t> workers = std::nullopt) noexcept;
		template <template <typename> typename Future = std::future, typename Fn>
		auto schedule(Fn&& func) -> std::pair<token, Future<decltype(func())>>
		{
			using return_t = decltype(func());

			auto tok{token{m_Invocables.size(), psl::view_ptr<scheduler>{this}}};

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

		void barriers(token token, psl::array<barrier> barrriers) noexcept;
		void consecutive(token target, psl::array<token> tokens) noexcept;

	  private:
		size_t m_Workers{4};
		size_t m_TokenOffset{0u};
		psl::array<details::packet> m_Invocables;
		std::atomic<int> m_RunningTasks;
	};
} // namespace psl::async2