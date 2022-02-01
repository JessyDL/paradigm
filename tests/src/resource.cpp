#include "resource/resource2.hpp"

#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

using namespace litmus;
namespace
{
	struct complex_data
	{
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << value;
		}
		static constexpr const char serialization_name[13] {"complex_data"};

		psl::serialization::property<"value", int> value {10};
	};

	struct trivial
	{
		trivial(core::resource::args_t<trivial>& data, int val = 99) { value = val; }

		int value {10};
	};

	struct complex
	{
		using resource_data_types = complex_data;
		complex() {}
		cppcoro::task<> construct(core::resource::args_t<complex>& data)
		{
			value = data.data.value;
			co_return;
		}
		cppcoro::task<> construct(core::resource::args_t<complex>& data, int val)
		{
			value = val;
			co_return;
		}

		int value {0};
	};

	struct lib_return_t
	{
		std::shared_ptr<psl::meta::library> lib;
		psl::UID complex_uid;
		cppcoro::static_thread_pool* pool {new cppcoro::static_thread_pool(12)};
	};

	lib_return_t create_library()
	{
		auto lib		 = std::make_shared<psl::meta::library>();
		auto complex_uid = psl::UID::generate();

		{
			psl::serialization::serializer s;
			psl::format::container cont {};

			complex_data data {99};
			s.serialize<psl::serialization::encode_to_format, complex_data>(data, cont);
			lib->create(complex_uid, cont.to_string());
		}
		return {lib, complex_uid};
	}

	auto t0 = suite<"resource creation", "core", "resource">().templates<tpack<trivial, complex>>() =
	  [data = create_library()]<typename T>() {
		  psl::UID complex_uid {data.complex_uid};
		  core::resource::cache_t cache {data.lib};

		  section<"async">() = [&]() {
			  section<"load">() = [&]() {
				  auto res	 = cache.load<T>(complex_uid);
				  auto&& val = cppcoro::sync_wait(res);
				  expect(val->value) == 99;
			  };

			  section<"copy">() = [&]() {
				  auto val = cache.load_now<T>(complex_uid);
				  auto copy1 {cppcoro::sync_wait(cache.copy(val))};
				  expect(copy1->value) == 99;
				  val->value = 20;
				  expect(copy1->value) == 99;
				  auto copy2 {cppcoro::sync_wait(cache.copy(val))};
				  expect(copy2->value) == 20;
			  };

			  section<"create">() = [&]() {
				  auto res	 = cache.create<T>(33);
				  auto&& val = cppcoro::sync_wait(res);
				  expect(val->value) == 33;
			  };
		  };

		  section<"immediate">() = [&]() {
			  section<"load">() = [&]() {
				  auto val = cache.load_now<T>(complex_uid);
				  expect(val->value) == 99;
			  };

			  section<"copy">() = [&]() {
				  auto val	   = cache.load_now<T>(complex_uid);
				  auto&& copy1 = cache.copy_now(val);
				  expect(copy1->value) == 99;
				  val->value = 20;
				  expect(copy1->value) == 99;
				  auto&& copy2 = cache.copy_now(val);
				  expect(copy2->value) == 20;
			  };

			  section<"create">() = [&]() {
				  auto val = cache.create_now<T>(33);
				  expect(val->value) == 33;
			  };
		  };

		  section<"async (custom scheduler)">() = [&]() {
			  section<"load">() = [&]() {
				  auto res	 = cache.load_on<T>(*data.pool, complex_uid);
				  auto&& val = cppcoro::sync_wait(res);
				  expect(val->value) == 99;
			  };

			  section<"copy">() = [&]() {
				  auto val = cache.load_now<T>(complex_uid);
				  auto copy1 {cppcoro::sync_wait(cache.copy_on(*data.pool, val))};
				  expect(copy1->value) == 99;
				  val->value = 20;
				  expect(copy1->value) == 99;
				  auto copy2 {cppcoro::sync_wait(cache.copy_on(*data.pool, val))};
				  expect(copy2->value) == 20;
			  };

			  section<"create">() = [&]() {
				  auto res	 = cache.create_on<T>(*data.pool, 33);
				  auto&& val = cppcoro::sync_wait(res);
				  expect(val->value) == 33;
			  };
		  };
	  };

	auto t1 = suite<"resource destruction", "core", "resource">() = []() {
		auto lib = std::make_shared<psl::meta::library>();
		core::resource::cache_t cache {lib};

		auto val = cache.create_now<trivial>(55);

		expect(cache.is_empty()) == false;
		cache.try_destroy(val);
		expect(cache.is_empty()) == true;
	};
}	 // namespace