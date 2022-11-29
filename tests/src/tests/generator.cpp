#include "tests/generator.hpp"
#include "psl/IDGenerator.hpp"

#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

namespace {
auto t0 = litmus::suite<"psl::generator">() = [](){
	using litmus::require;
	psl::generator<uint64_t> generator;

	require(generator.capacity()) == generator.available();
	require(generator.capacity()) == std::numeric_limits<uint64_t>::max();

	size_t accumulated = 0;
	uint64_t id{1523365};
	id = generator.create(100);
	accumulated += 100;
	require(id) == 0;
	require(generator.capacity()) == generator.available() + accumulated;
	require(generator.size()) == accumulated;

	require(generator.destroy(1));
	accumulated -= 1;
	require(generator.capacity()) == generator.available() + accumulated;
	require(generator.size()) == accumulated;
	for(auto i = 0; i < 100; ++i)
	{
		if(i == 1)
			require(!generator.valid(i));
		else
			require(generator.valid(i));
	}

	id = generator.create(15);
	accumulated += 15;
	require(id) == 100;
	require(generator.capacity()) == generator.available() + accumulated;
	require(generator.size()) == accumulated;

	id = generator.create(1);
	accumulated += 1;
	require(id) == 1;
	require(generator.capacity()) == generator.available() + accumulated;
	require(generator.size()) == accumulated;

	require(generator.destroy(1));
	accumulated -= 1;
	require(generator.capacity()) == generator.available() + accumulated;
	require(generator.size()) == accumulated;
	require(generator.destroy(2));
	accumulated -= 1;
	require(generator.capacity()) == generator.available() + accumulated;
	require(generator.size()) == accumulated;

	std::vector<std::pair<size_t, size_t>> allocated;
	for(size_t i = 0u; i < 10240; ++i)
	{
		size_t size = (std::rand() % 128) + 1;
		allocated.emplace_back(generator.create(size), size);
		accumulated += size;
	}
	require(generator.capacity()) == generator.available() + accumulated;
	require(generator.size()) == accumulated;
	
	for(size_t i = 0u; i < 10240; ++i)
	{
		if(std::rand() % 3 == 0 || allocated.size() < 128)
		{
			size_t size = (std::rand() % 128) + 1;
			allocated.emplace_back(generator.create(size), size);
			accumulated += size;
		}
		else
		{
			auto it = std::next(std::begin(allocated), std::rand() % allocated.size());
			size_t size = (it->second > 10)?(std::rand() % it->second) + 1: it->second;

			require(generator.destroy(it->first, size));
			accumulated -= size;
			std::pair<size_t, size_t> new_pair{it->first + size, it->second - size};
			allocated.erase(it);
			if(new_pair.second > 0)
			{
				require(generator.valid(new_pair.first));
				require(!generator.valid(new_pair.first - 1));
				allocated.emplace_back(new_pair);
			}

		}
	}
	require(generator.capacity()) == generator.available() + accumulated;
	require(generator.size()) == accumulated;
};
}