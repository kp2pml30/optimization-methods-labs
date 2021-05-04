#include "opt-methods/math/SkylineMatrix.hpp"

#include <sstream>
#include <string>
#include <iterator>

int main()
{
	auto printVector = [](std::ostream& o, Vector<double> const& vec) {
		o << '{';
		for (int i = 0; i < (int)vec.size(); i++)
		{
			o << vec[i];
			if (i != (int)vec.size() - 1)
				o << ", ";
		}
		o << '}';
	};

	using namespace std::literals;

	/*
	std::istringstream ss(R"_delim(3
0 0 1 2
10 -0.001 5
0 2.5
-7 6
)_delim"s);
	std::istringstream ss(R"_delim(3
0 0 1 3
1 4 7
-1 -5 2
2 -3 2
)_delim"s);
	*/
	std::istringstream ss(R"_delim(4
0 0 0 1 2
1 2 1 2
3 1
-1 2
)_delim"s);
	// Vector<double> b = {7, 6.001, 2.5};
	// Vector<double> b = {2, 1, 3};
	Vector<double> b = {1, 1, 1, 1};

	auto A = SkylineMatrix<double>::ReadFrom(ss);
	std::cout << "A:\n";
	PrintDense(std::cout, A);
	std::cout << "b: ";
	printVector(std::cout, b);
	std::cout << "\n\n";

	auto x = std::move(A).solveSystem(b);
	std::cout << "x: ";
	printVector(std::cout, x);
	std::cout << std::endl;

	// std::cout << "\nas profile\n" << m;
	return 0;
}
