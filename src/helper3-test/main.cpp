#include "opt-methods/math/DenseMatrix.hpp"
#include "opt-methods/math/SkylineMatrix.hpp"
#include "opt-methods/math/RowColumnSymMatrix.hpp"

#include <sstream>
#include <string>
#include <iterator>
#include <random>

int main()
{
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
	std::istringstream ai("0 0 0 1 2"), di("1 2 1 2"), al("3 1"), au("-1 2");
	// Vector<double> b = {7, 6.001, 2.5};
	// Vector<double> b = {2, 1, 3};
	Vector<double> b = {1, 1, 1, 1};

	auto A = SkylineMatrix<double>::ReadFrom(ai, di, al, au);
	std::cout << "A:\n";
	PrintDense(std::cout, A);
	std::cout << "A (dense):\n";
	auto a_dense = static_cast<DenseMatrix<double>>(A);
	for (int i = 0; i < (int)a_dense.Dims(); i++, std::cout << '\n')
		for (int j = 0; j < (int)a_dense.Dims(); j++, std::cout << '\t')
			std::cout << *a_dense.IteratorAt(i, j);
	using namespace util;
	std::cout << "b: " << b << "\n\n";

	auto x = std::move(A).SolveSystem(b);
	std::cout << "x: " << x << '\n';
	std::cout << "x (dense): " << std::move(a_dense).SolveSystem(b) << '\n';

	{
		auto A = util::DiagonallyDominant(
		    MatrixGenerator<double, SkylineMatrix<double>>(),
		    10, 0.1, {1, 2, 3, -1, -2, -3},
		    std::function<double (std::default_random_engine&)>([distr = std::uniform_int_distribution<int>(-4, 0)](auto& engine) mutable -> double {
			    return distr(engine);
		    }));
		auto a_dense = static_cast<DenseMatrix<double>>(A);
		std::cout << "A: ";
		PrintDense(std::cout, A);
		std::cout << '\n';
		Vector<double> x_star(0.0, 10);
		std::iota(std::begin(x_star), std::end(x_star), 1);
		auto b = A * x_star;

		auto x = std::move(A).SolveSystem(b);
		std::cout << "x*: " << x_star << '\n';
		std::cout << "x: " << x << '\n';
		std::cout << "x (dense): " << std::move(a_dense).SolveSystem(b) << '\n';
	}

	{
		auto A = util::Hilbert(MatrixGenerator<double, SkylineMatrix<double>>(), 10, {1, 2, 3, -1, -2, -3});
		auto a_dense = static_cast<DenseMatrix<double>>(A);
		std::cout << "A: ";
		PrintDense(std::cout, A);
		std::cout << '\n';
		Vector<double> x_star(0.0, 10);
		std::iota(std::begin(x_star), std::end(x_star), 1);
		auto b = A * x_star;

		auto x = std::move(A).SolveSystem(b);
		std::cout << "x*: " << x_star << '\n';
		std::cout << "x: " << x << '\n';
		std::cout << "x (dense): " << std::move(a_dense).SolveSystem(b) << '\n';
	}

	{
		auto A = util::DiagonallyDominant(
		    MatrixGenerator<double, RowColumnSymMatrix<double>>(),
		    10, 0.1, {1, 2, 3, -1, -2, -3},
		    std::function<double (std::default_random_engine&)>([distr = std::uniform_int_distribution<int>(-4, 0)](auto& engine) mutable -> double {
			    return distr(engine);
		    }));
		auto a_dense = static_cast<DenseMatrix<double>>(A);
		std::cout << "A: ";
		PrintDense(std::cout, a_dense);
		std::cout << '\n';
		Vector<double> x_star(0.0, 10);
		std::iota(std::begin(x_star), std::end(x_star), 1);
		auto b = A * x_star;

		auto x = std::move(A).SolveSystem(b);
		std::cout << "x*: " << x_star << '\n';
		std::cout << "x: " << x << '\n';
		std::cout << "x (dense): " << std::move(a_dense).SolveSystem(b) << '\n';
	}

	{
		auto A = util::Hilbert(MatrixGenerator<double, RowColumnSymMatrix<double>>(), 10, {1, 2, 3, -1, -2, -3});
		auto a_dense = static_cast<DenseMatrix<double>>(A);
		std::cout << "A: ";
		PrintDense(std::cout, a_dense);
		std::cout << '\n';
		Vector<double> x_star(0.0, 10);
		std::iota(std::begin(x_star), std::end(x_star), 1);
		auto b = A * x_star;

		auto x = std::move(A).SolveSystem(b);
		std::cout << "x*: " << x_star << '\n';
		std::cout << "x: " << x << '\n';
		std::cout << "x (dense): " << std::move(a_dense).SolveSystem(b) << '\n';
	}
	// std::cout << "\nas profile\n" << m;
	return 0;
}
