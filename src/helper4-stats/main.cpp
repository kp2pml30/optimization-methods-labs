#include "opt-methods/newton/Newton.hpp"
#include "opt-methods/math/BisquareFunction.hpp"

#include <iostream>

int main()
{
	using P = Vector<double>;
	using V = double;
	auto newton = Newton<P, V>(1e-5);
	auto func = QuadraticFunction2d<double>(2.5, 3, 4, 0.1, -0.5, 10);
	std::cout << static_cast<std::string>(func) << std::endl;
	auto gen = newton(func, {Vector<double>{{1, 1}}, 5});
	while (gen.next())
	{
		auto res = gen.getValue();
		std::cout << "p :";
		for (std::size_t i = 0; i < res.p.size(); i++)
			std::cout << ' '<< res.p[i];
		std::cout << '\n';
	}
	return 0;
}
