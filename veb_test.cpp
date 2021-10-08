#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch.hpp"
#include "veb.h"
#include <random>
#include <algorithm>
#define LOG2_U 20

namespace
{
	std::vector<doo::u64> gen_permut(doo::u64 start, doo::u64 end)
	{
		std::vector<doo::u64> out;
		out.reserve(end - start);
		for (doo::u64 i = start; i <= end; i++)
		{
			out.push_back(i);
		}
		auto dre = std::default_random_engine{};
		std::shuffle(out.begin(), out.end(), dre);
		return out;
	}

	void insert(doo::veb<LOG2_U>& v, const std::vector<doo::u64>& to_insert)
	{
		for (doo::u64 key : to_insert)
		{
			v.insert(key);
		}
	}

	void del(doo::veb<LOG2_U>& v, const std::vector<doo::u64>& to_delete)
	{
		for (doo::u64 key : to_delete)
		{
			v.del(key);
		}
	}

	std::vector<doo::u64> sort(doo::veb<LOG2_U>& v, const std::vector<doo::u64>& to_sort)
	{
		std::vector<doo::u64> sorted;
		insert(v, to_sort);
		auto temp = v.get_min();

		while (temp.has_value())
		{
			sorted.push_back(temp.value());
			temp = v.succ(temp.value());
		}

		return sorted;
	}
}

TEST_CASE("Insert & Delete")
{
	SECTION("Insert and Delete Min")
	{
		doo::veb<LOG2_U> v;
		int end = 500000;
		int start = 0;
		auto to_insert = gen_permut(start, end);
		insert(v, to_insert);

		for (int i = start; i <= end; i++)
		{
			auto min = v.remove_min();
			REQUIRE(min.has_value());
			REQUIRE(min.value() == i);
		}
	}

	SECTION("Duplicate Insert")
	{
		doo::veb<LOG2_U> v;
		v.insert_if_not_exists(0);
		v.insert_if_not_exists(0);
		v.insert_if_not_exists(2);
		v.insert_if_not_exists(2);
		v.insert_if_not_exists(1);
		v.insert_if_not_exists(1);
		REQUIRE(v.member(0));
		REQUIRE(v.member(1));
		REQUIRE(v.member(2));
		v.del(0);
		v.del(2);
		v.del(1);
		REQUIRE(!v.member(0));
		REQUIRE(!v.member(1));
		REQUIRE(!v.member(2));
	}
}

TEST_CASE("Delete")
{
	doo::veb<LOG2_U> v;
	int start = 0; int end = 500000;
	auto to_insert = gen_permut(start, end);
	insert(v, to_insert);
	del(v, to_insert);
	REQUIRE(v.empty());
}



TEST_CASE("Successor")
{
	doo::veb<LOG2_U> v;
	v.insert(0);
	v.insert(15);
	v.insert(16);
	auto s = v.succ(4);
	REQUIRE(s.has_value());
	REQUIRE(s.value() == 15);

	s = v.succ(0);
	REQUIRE(s.has_value());
	REQUIRE(s.value() == 15);

	s = v.succ(16);
	REQUIRE(!s.has_value());

	s = v.succ(17);
	REQUIRE(!s.has_value());
}

TEST_CASE("Predecessor")
{
	doo::veb<LOG2_U> v;
	v.insert(1);
	v.insert(15);
	v.insert(16);
	auto s = v.pred(13);
	REQUIRE(s.has_value());
	REQUIRE(s.value() == 1);

	s = v.pred(16);
	REQUIRE(s.has_value());
	REQUIRE(s.value() == 15);

	s = v.pred(200);
	REQUIRE(s.has_value());
	REQUIRE(s.value() == 16);

	s = v.pred(346);
	REQUIRE(s.has_value());
	REQUIRE(s.value() == 16);

	s = v.pred(1);
	REQUIRE(!s.has_value());
}

TEST_CASE("Member")
{
	doo::veb<LOG2_U> v;
	REQUIRE(!v.member(0));
	v.insert(10);
	v.insert(3);
	v.insert(2);
	v.insert(1);
	v.insert(0);
	REQUIRE(v.member(3));
	REQUIRE(v.member(2));
	REQUIRE(v.member(1));
	REQUIRE(v.member(0));
	REQUIRE(!v.member(5));
	REQUIRE(!v.member(7));
}

TEST_CASE("Sorting")
{
	auto ti = gen_permut(0, 500000);

	// sort the permutation for asserting that veb sort worked
	std::vector<doo::u64> sorted = ti;
	std::sort(sorted.begin(), sorted.end());
	doo::veb<LOG2_U> v;
	auto vebSorted = sort(v, ti);
	REQUIRE(sorted.size() == vebSorted.size());
	REQUIRE(sorted == vebSorted);
}

TEST_CASE("Benchmark")
{
	// generate a permultation of random ids for testing
	auto ti = gen_permut(0, 500000);

	// sort the permutation for asserting that veb sort worked
	std::vector<doo::u64> sorted = ti;
	std::sort(sorted.begin(), sorted.end());

	BENCHMARK("Insert Performance")
	{
		doo::veb<LOG2_U> v;
		REQUIRE(v.empty());
		insert(v, ti);
		REQUIRE(!v.empty());
	};

	BENCHMARK("Insert & Delete Performance")
	{
		doo::veb<LOG2_U> v;
		REQUIRE(v.empty());
		insert(v, ti);
		REQUIRE(!v.empty());
		del(v, ti);
		REQUIRE(v.empty());
	};

	BENCHMARK("Sort Performance")
	{
		doo::veb<LOG2_U> v;
		auto vebSorted = sort(v, ti);
	};
}
