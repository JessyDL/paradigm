#include <litmus/expect.hpp>
#include <litmus/section.hpp>
#include <litmus/suite.hpp>

#include "psl/sparse_array.hpp"

namespace {
auto t0_sparse = litmus::suite<"psl::sparse_array">() = []() {
	using namespace litmus;
	section<"basic_operations">() = []() {
		psl::sparse_array<int> array {};
		require(array.size()) == 0;
		require(array.empty()) == true;
		array.insert(0, 42);
		require(array.size()) == 1;
		require(array.empty()) == false;
		require(array.at(0)) == 42;
		array.insert(2, 84);
		require(array.size()) == 2;
		require(array.at(2)) == 84;
		array.insert(1, 21);
		require(array.size()) == 3;
		require(array.at(1)) == 21;
		require(array.at(0)) == 42;
		require(array.at(2)) == 84;
		array.erase(1);
		require(array.size()) == 2;
		require(array.at(0)) == 42;
		require(array.at(2)) == 84;
		require(array.contains(1)) == false;
		array.clear();
		require(array.size()) == 0;
		require(array.empty()) == true;
	};

	section<"chunk_bound_ops">() = []() {
		psl::sparse_array<int> array {};
		std::vector<int> values {10, 20, 30, 40, 50};
		std::vector<size_t> indices {4095, 4096, 4097, 8191, 8192};
		array.insert(indices.begin(), indices.end(), values.begin(), values.end());
		require(array.size()) == 5;
		require(array.at(4095)) == 10;
		require(array.at(4096)) == 20;
		require(array.at(4097)) == 30;
		require(array.at(8191)) == 40;
		require(array.at(8192)) == 50;
		array.erase(indices.begin(), indices.begin() + 2);
		require(array.size()) == 3;
		require(!array.contains(4095));
		require(!array.contains(4096));
		require(array.contains(4097));
		require(array.contains(8191));
		require(array.contains(8192));
		require(array.at(4097)) == 30;
		require(array.at(8191)) == 40;
		require(array.at(8192)) == 50;

		require(!array.assign(4096, 100));	  // should update if already present
		require(array.size()) == 3;
		require(array.assign(4097, 100));
		require(array.at(4097)) == 100;
		require(!array.set(8192, 200));	   // should return false as set only returns the count of new inserts
		require(array.size()) == 3;
		require(array.at(8192)) == 200;
		array.set(1234, 300);	 // should insert
		require(array.size()) == 4;
		require(array.at(1234)) == 300;
		require(!array.try_erase(1));
		require(array.try_erase(1234));
		require(array.size()) == 3;
		require(!array.assign(999, 400));

		auto arr_indices = array.indices();
		require(array.erase(arr_indices.begin(), arr_indices.end()) == arr_indices.size());
	};
};
auto t1_sparse = litmus::suite<"psl::sparse_indice_array">() = []() {
	using namespace litmus;
	section<"basic_operations">() = []() {
		psl::sparse_indice_array<size_t> array {};
		require(array.size()) == 0;
		require(array.empty()) == true;
		array.insert(0);
		require(array.size()) == 1;
		require(array.empty()) == false;
		array.insert(2);
		require(array.size()) == 2;
		array.insert(1);
		require(array.size()) == 3;
		require(array.contains(1));
		require(array.contains(0));
		require(array.contains(2));
		array.erase(1);
		require(array.size()) == 2;
		require(array.contains(0));
		require(array.contains(2));
		require(!array.contains(1));
		array.clear();
		require(array.size()) == 0;
		require(array.empty()) == true;
	};
};
}	 // namespace
