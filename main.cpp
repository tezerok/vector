#include "vector.h"
#include <iostream>

// This file contains (mostly manual) tests of the vector

// Helper class to test vector's exception safety
// Throws an exception on 'throwN'-th construction
template <int throwN>
struct CtorThrower {
	int i;

	CtorThrower(int i = 0) : i(i) {}

	// Mark this ctor as noexcept to ensure vector's nothrow guarantee
	CtorThrower(CtorThrower&& x) {
		i = x.i;
		x.i = 0;

		static int instanceN = 0;
		if (++instanceN == throwN)
			throw 123;
	}

	// Uncomment this ctor to test out strong guarantee
	// Comment this ctor to test out basic guarantee
	CtorThrower(const CtorThrower& x) {
		i = x.i;

		static int instanceN = 0;
		if (++instanceN == throwN)
			throw 123;
	}
};

int main()
{
	// Test basic operations with a trivial data type
	{
		vector<int> v;
		for (int i = 0; i < 10; ++i)
			v.push_back(10-i);

		for (auto i : v)
			std::cout << i << " ";
		std::cout << "\n";

		std::sort(std::begin(v), std::end(v));

		for (auto i : v)
			std::cout << i << " ";
		std::cout << "\n";

		v.erase(v.begin()+4);
		v.erase(v.begin()+4);
		v.erase(v.begin());
		v.erase(v.end()-1);

		for (auto i : v)
			std::cout << i << " ";
		std::cout << "\n";

		v.insert(v.begin(), 1);
		v.emplace(v.end(), 10);
		v.emplace(v.begin()+5, 6);
		v.emplace(v.begin()+5, 5);

		for (auto i : v)
			std::cout << i << " ";
		std::cout << "\n";

		vector<int> vv = {11, 22, 33};
		vv.emplace_back(44);
		vv.reserve(64);
		vv.emplace_back(55);
		vv.resize(8);
		for (auto i : vv)
			std::cout << i << " ";
		std::cout << "\n";
	}

	// Test exception safety
	{
		vector<CtorThrower<3>> v;
		try {
			v.emplace_back(1);
			v.emplace_back(2);
			v.emplace_back(3);
			v.emplace_back(4);
			v.emplace_back(5);
		} catch(...) {
			std::cout << v.size() << "\n";
			for (auto& x : v)
				std::cout << x.i << " ";
			std::cout << "\n";
		}
	}
}
