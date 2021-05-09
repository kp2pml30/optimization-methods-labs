#include <tuple>
#include <functional>
#include <cmath>

namespace
{
	template<typename T>
	using Modifier = std::function<T(T const&)>;

	template<std::size_t i, typename ...Args>
	bool Advance(std::tuple<Args&...> cur, std::tuple<Args...> const& begins, std::tuple<Args...> const& ends, std::tuple<Modifier<Args>...> const& modifiers)
	{
		if constexpr (i == sizeof...(Args))
			return false;
		else
		{
			if (Advance<i + 1>(cur, begins, ends, modifiers))
				return true;
			auto& c = std::get<i>(cur);
			c = std::get<i>(modifiers)(c);
			if (c >= std::get<i>(ends))
			{
				c = std::get<i>(begins);
				return false;
			}
			else
			{
				return true;
			}
		}
	}

	template<typename ...Args>
	void GetStatsVectorizedAdvance(std::function<void(Args const&...)> test, std::tuple<Args...> begins, std::function<bool(Args&...)> const& modifiers)
	{
		do
		{
			std::apply(test, begins);
		} while (std::apply(modifiers, begins));
	}

	template<typename ...Args>
	void GetStats(std::function<void(Args const&...)> test, std::tuple<Args...> const& begins, std::tuple<Args...> const& ends, std::tuple<Modifier<Args>...> const& modifiers)
	{
		auto myadvance = std::function([&](Args&... a) { return Advance<0>(std::tie(a...), begins, ends, modifiers); });
		GetStatsVectorizedAdvance(test, begins, myadvance);
	}
}

#include <iostream>

int main()
{
	// examples:
	std::cout << " -= example 1 =-\n";
	auto test = [](int const& i, double const& j) {
		std::cout << "i=" << i << "\tj=" << j << std::endl;
	};
	GetStats(std::function(test),
			std::make_tuple(0, 1.0),
			std::make_tuple(2, 16.0),
			std::make_tuple(std::function([](int const& i) { return i + 1; }),
				std::function([](double const& j) { return j * 2; })));
	std::cout << " -= example 2 =-\n";
	GetStatsVectorizedAdvance(std::function(test),
			std::make_tuple(0, 1.0),
			std::function([](int& i, double& e) {
				if (i > 10)
					return false;
				i++;
				e = std::exp(i);
				return true;
			}));
	return 0;
}
