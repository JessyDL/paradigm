#pragma once
#include "stdafx_benchmarks.h"
#include "spdlog/spdlog.h"
#include <vector>
#include <cstdint>

#include <future>

#define INTERNAL_UNIQUE_NAME_LINE2(name, counter) name##counter
#define INTERNAL_UNIQUE_NAME_LINE(name, counter) INTERNAL_UNIQUE_NAME_LINE2(name, counter)
#define INTERNAL_UNIQUE_NAME(name) INTERNAL_UNIQUE_NAME_LINE(name, __COUNTER__)

//#define INTERNAL_GET_TYPE(TARGET) typename std::invoke_result_t<typename
// std::decay<decltype(std::function(TARGET))>::type>
#define INTERNAL_GET_TYPE_REF(TARGET) typename TARGET&
#define INTERNAL_GET_TYPE_MOVE(TARGET) typename TARGET&&
#define INTERNAL_GET_TYPE(TARGET, REUSE)                                                                               \
	typename std::conditional<REUSE, INTERNAL_GET_TYPE_REF(TARGET), INTERNAL_GET_TYPE_MOVE(TARGET)>::type

#define INTERNAL_REGISTER_BENCHMARK2(NAME, TARGET, DESCRIPTION)                                                        \
	static benchmarks::details::proxy<void> NAME(TARGET, DESCRIPTION);

#define INTERNAL_REGISTER_BENCHMARK(NAME, DESCRIPTION)                                                                 \
	INTERNAL_REGISTER_BENCHMARK2(INTERNAL_UNIQUE_NAME(BENCHMARK_TEST_FUNC_REGISTER), NAME, DESCRIPTION)

#define INTERNAL_CREATE_BENCHMARK(NAME, DESCRIPTION)                                                                   \
	static void NAME();                                                                                                \
	INTERNAL_REGISTER_BENCHMARK(NAME, DESCRIPTION)                                                                     \
	static void NAME()

#define INTERNAL_BENCHMARK_1(DESCRIPTION)                                                                              \
	INTERNAL_CREATE_BENCHMARK(INTERNAL_UNIQUE_NAME(BENCHMARK_TEST_FUNC), DESCRIPTION)


#define INTERNAL_REGISTER_BENCHMARK2_MOCKUP(NAME, TARGET, DESCRIPTION, MOCKUP, REUSE)                                  \
	static benchmarks::details::proxy<INTERNAL_GET_TYPE(MOCKUP, REUSE)> NAME(TARGET, DESCRIPTION);

#define INTERNAL_REGISTER_BENCHMARK_MOCKUP(NAME, DESCRIPTION, MOCKUP, REUSE)                                           \
	INTERNAL_REGISTER_BENCHMARK2_MOCKUP(INTERNAL_UNIQUE_NAME(BENCHMARK_TEST_FUNC_REGISTER), NAME, DESCRIPTION, MOCKUP, \
										REUSE)

#define INTERNAL_CREATE_BENCHMARK_PARAMS(NAME, DESCRIPTION, MOCKUP, REUSE)                                             \
	static void NAME(INTERNAL_GET_TYPE(MOCKUP, REUSE) data);                                                           \
	INTERNAL_REGISTER_BENCHMARK_MOCKUP(NAME, DESCRIPTION, MOCKUP, REUSE)                                               \
	static void NAME(INTERNAL_GET_TYPE(MOCKUP, REUSE) data)

#define INTERNAL_BENCHMARK_2(DESCRIPTION, MOCKUP)                                                                      \
	INTERNAL_CREATE_BENCHMARK_PARAMS(INTERNAL_UNIQUE_NAME(BENCHMARK_TEST_FUNC), DESCRIPTION, MOCKUP, false)

#define INTERNAL_BENCHMARK_3(DESCRIPTION, MOCKUP, REUSE)                                                               \
	INTERNAL_CREATE_BENCHMARK_PARAMS(INTERNAL_UNIQUE_NAME(BENCHMARK_TEST_FUNC), DESCRIPTION, MOCKUP, REUSE)


#define FUNC_CHOOSER(_f1, _f2, _f3, _f4, ...) _f4
#define FUNC_RECOMPOSER(argsWithParentheses) FUNC_CHOOSER argsWithParentheses
#define CHOOSE_FROM_ARG_COUNT(...)                                                                                     \
	FUNC_RECOMPOSER((__VA_ARGS__, INTERNAL_BENCHMARK_3, INTERNAL_BENCHMARK_2, INTERNAL_BENCHMARK_1, ))
#define NO_ARG_EXPANDER() , , INTERNAL_BENCHMARK_0
#define MACRO_CHOOSER(...) CHOOSE_FROM_ARG_COUNT(NO_ARG_EXPANDER __VA_ARGS__())


#define BENCHMARKC(...) MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#define RUN_ALL_BENCHMARKS(LOGGER) benchmarks::details::m_Handler.run(LOGGER);
namespace benchmarks::details
{
	class runner
	{
	  public:
		template <typename F>
		runner(F&& fnc, const size_t iterations)
		{
			std::chrono::time_point<std::chrono::high_resolution_clock> start{};
			results.reserve(iterations);
			for(size_t i = 0; i < iterations; ++i)
			{
				start = std::chrono::high_resolution_clock::now();
				std::invoke(fnc);
				results.emplace_back(
					std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() * 1000.0);
			}
			std::sort(std::begin(results), std::end(results));
		}

		template <typename MT, typename F>
		runner(F&& fnc, const size_t iterations, MT*, bool)
		{
			std::chrono::time_point<std::chrono::high_resolution_clock> start{};
			results.reserve(iterations);

			MT mock{};
			for(size_t i = 0; i < iterations; ++i)
			{
				start = std::chrono::high_resolution_clock::now();
				std::invoke(std::forward<F>(fnc), mock);
				results.emplace_back(
					std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() * 1000.0);
			}

			std::sort(std::begin(results), std::end(results));
		}
		template <typename MT, typename F>
		runner(F&& fnc, const size_t iterations, MT*)
		{
			std::chrono::time_point<std::chrono::high_resolution_clock> start{};
			results.reserve(iterations);

			for(size_t i = 0; i < iterations; ++i)
			{
				MT mock{};
				start = std::chrono::high_resolution_clock::now();
				std::invoke(std::forward<F>(fnc), std::move(mock));
				results.emplace_back(
					std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() * 1000.0);
			}

			std::sort(std::begin(results), std::end(results));
		}

		double time(size_t percentile)
		{
			percentile   = std::max<size_t>(std::min<size_t>(100, percentile), 0);
			size_t index = (size_t)(((double)(results.size() - 1) / 100.0) * (double)percentile);
			return results[index];
		}

		template <typename... size_t>
		void log(spdlog::logger& logger, std::string& description, size_t... percentiles)
		{
			std::string format{description + "\n    {} iterations [ "};
			(format.append(std::to_string(percentiles) + "% {}ms | "), ...);
			format.resize(format.size() - 2);
			format.append(" ]");
			logger.info(format.c_str(), results.size(), time(percentiles)...);
		};

	  private:
		std::vector<double> results;
	};

	class proxy_base;

	class handler
	{
	  public:
		void run(spdlog::logger& logger, bool multithreaded = false);

		void add(proxy_base* proxy) { m_Functions.emplace_back(proxy); }

		std::vector<proxy_base*> m_Functions;
	};


	static handler m_Handler{};

	class proxy_base
	{
	  public:
		virtual void operator()(spdlog::logger& logger, size_t iterations) = 0;
	};

	template <typename T>
	class proxy : public proxy_base
	{
	  public:
		proxy(std::function<void(T)> func, std::string description) : func(func), description(description)
		{
			m_Handler.add(this);
		}
		void operator()(spdlog::logger& logger, size_t iterations) override
		{
			using type = typename std::decay<T>::type;
			type* x	= nullptr;
			if constexpr(std::is_same_v<T, type&>)
			{
				runner b{func, iterations, x, true};
				b.log(logger, description, 0, 25, 50, 80, 95, 98, 100);
			}
			else
			{
				runner b{func, iterations, x};
				b.log(logger, description, 0, 25, 50, 80, 95, 98, 100);
			}
		}

		std::function<void(T)> func;
		std::string description;
	};

	template <>
	class proxy<void> : public proxy_base
	{
	  public:
		template <typename X>
		proxy(std::function<void()> func, std::string description, X) : func(func), description(description)
		{
			m_Handler.add(this);
		}

		proxy(std::function<void()> func, std::string description) : func(func), description(description)
		{
			m_Handler.add(this);
		}
		void operator()(spdlog::logger& logger, size_t iterations) override
		{
			runner b{func, iterations};
			b.log(logger, description, 0, 25, 50, 80, 95, 98, 100);
		}

		std::function<void()> func;
		std::string description;
	};


	void handler::run(spdlog::logger& logger, bool multithreaded)
	{
		if(multithreaded)
		{
			std::vector<std::future<void>> benchmarks;
			for(auto& fn : m_Functions)
			{
				benchmarks.emplace_back(
					std::async(std::launch::async, &proxy_base::operator(), fn, std::ref(logger), 100));
			}

			for(auto& future : benchmarks) future.wait();
		}
		else
		{
			for(auto& fn : m_Functions)
			{
				fn->operator()(logger, 100);
			}
		}
	}
} // namespace benchmarks::details