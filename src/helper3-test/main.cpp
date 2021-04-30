#include "opt-methods/math/ProfileMatrix.hpp"

int main()
{
	auto m = ProfileMatrix<double>::ReadFrom(std::cin);
	PrintDense(std::cout, m);
	std::cout << "\nas profile\n" << m;
	return 0;
}
