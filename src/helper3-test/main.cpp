#include "opt-methods/math/SkylineMatrix.hpp"

int main()
{
	auto m = SkylineMatrix<double>::ReadFrom(std::cin);
	PrintDense(std::cout, m);
	std::cout << "\nas profile\n" << m;
	return 0;
}
