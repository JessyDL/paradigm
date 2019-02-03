#include "stdafx_tests.h"
#include "tests/generator.h"
#include "IDGenerator.h"

TEST_CASE("psl::generator", "[psl]")
{
	psl::generator<uint64_t> generator;

	REQUIRE(generator.capacity() == generator.available());
	REQUIRE(generator.capacity() == std::numeric_limits<uint64_t>::max());
	
	uint64_t id{1523365};
	id = generator.create(100);
	REQUIRE(id == 0);
	REQUIRE(generator.capacity() == generator.available() + 100);
	REQUIRE(generator.size() == 100);

	REQUIRE(generator.destroy(1));
	REQUIRE(generator.capacity() == generator.available() + 99);
	REQUIRE(generator.size() == 99);
	for(auto i = 0; i < 100; ++i)
	{
		if(i == 1)
			REQUIRE(!generator.valid(i));
		else
			REQUIRE(generator.valid(i));
	}

	id = generator.create(15);
	REQUIRE(id == 100);
	REQUIRE(generator.capacity() == generator.available() + 114);
	REQUIRE(generator.size() == 114);



	id = generator.create(1);
	REQUIRE(id == 1);
	REQUIRE(generator.capacity() == generator.available() + 115);
	REQUIRE(generator.size() == 115);

	REQUIRE(generator.destroy(1));
	REQUIRE(generator.capacity() == generator.available() + 114);
	REQUIRE(generator.size() == 114);
	REQUIRE(generator.destroy(2));
	REQUIRE(generator.capacity() == generator.available() + 113);
	REQUIRE(generator.size() == 113);

}