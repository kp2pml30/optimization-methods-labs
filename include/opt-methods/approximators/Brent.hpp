#pragma once

#include <numbers>
#include <cassert>
#include <optional>

#include <QtCharts/QLineSeries>

#include "opt-methods/solvers/BaseApproximator.hpp"
#include "Parabolic.hpp"

template<std::floating_point From, typename To> requires std::is_convertible_v<From, To>
class BrentApproximator : public BaseApproximator<From, To, BrentApproximator<From, To>>
{
	using BaseT = BaseApproximator<From, To, BrentApproximator>;
private:
public:
	using P = From;
	using V = To;

	struct IterationData : BaseT::IterationData
	{
		typename ParabolicApproximator<P, V>::IterationData parabola;
		bool useParabola;
	};

	static char const* name() noexcept { return "brent"; }

	static constexpr P tau = std::numbers::phi_v<P> - 1;

	P epsilon;

	BrentApproximator(P epsilon) : epsilon(epsilon) {}

	static void draw_impl(BoundsWithValues<P, V> r, IterationData const& data, QtCharts::QChart &chart)
	{
		if (data.useParabola)
			ParabolicApproximator<P, V>::draw_impl(r, data.parabola, chart);
		else
			Charting::addToChart(
					&chart,
					Charting::drawPoints({QPointF{data.parabola.bar.p, data.parabola.bar.v}}, "Chosen optimization point (u)"));
	}

	template<Function<P, V> F>
	ApproxGenerator<P, V> begin_impl(F func, BoundsWithValues<P, V> r, IterationData& data)
	{
		using std::abs;
		using std::lerp;
		using std::copysign;

		assert(r.l.p < r.r.p);

		auto a = r.l.p, b = r.r.p;
		///  a          c
		auto fa = r.l.v, fb = r.r.v;
		auto x = lerp(a, b, 1 - tau), premin = x, last_premin = x;
		///  x                        w            v
		auto fx = func(x), fpm = fx, flpm = fx;
		auto cur_step = b - a, last_step = cur_step;
		///   d                   e

		while (true)
		{
			auto epsilon = this->epsilon * (abs(x) + P(0.1));
			if (abs(x - (a + b) / 2) + (b - a) / 2 <= 2 * epsilon) break;

			auto step = last_step;
			///   g
			last_step = cur_step;

			auto all_uneq = []<std::floating_point T>(T a, T b, T c) {
				return a != b && a != c && b != c;
			};

			P u;
			{
				data.useParabola = false;

				std::optional<P> optu;
				if (all_uneq(x, premin, last_premin) && all_uneq(fx, fpm, flpm))
				{
					// can use parabolic interpolation
					auto [a0, a1, a2, u_cand] =
							ParabolicApproximator<P, V>::approxParabola(x, fx, premin, fpm, last_premin, flpm);
					if (u_cand >= a + epsilon && u_cand <= b - epsilon && abs(u_cand - x) < step / 2)
					{
						// accept u
						if (u_cand - a < 2 * epsilon || b - u_cand < 2 * epsilon)
							u_cand = x - copysign(epsilon, x - (a + b) / 2); // don't stick
						optu = u_cand;

						data.useParabola = true;
						data.parabola    = {{}, a0, a1, a2, x, premin, last_premin, {P(0), V(0)}};
					}
				}

				if (!optu.has_value())
				{
					// golden section
					if (x < (a + b) / 2)
					{
						optu      = lerp(x, b, 1 - tau);
						last_step = b - x;
					}
					else
					{
						optu      = lerp(a, x, tau);
						last_step = x - a;
					}
				}

				u = *optu;
			}

			if (abs(u - x) < epsilon)
				u = x + copysign(epsilon, u - x); // don't stick
			cur_step = abs(u - x);

			auto fu = func(u);
			data.parabola.bar = {u, fu};
			if (fu <= fx)
			{
				if (u >= x)
					a = x, fa = fx;
				else
					b = x, fb = fx;
				last_premin = premin, premin = x, x = u;
				flpm = fpm, fpm = fx, fx = fu;
			}
			else
			{
				if (u >= x)
					b = u, fb = fu;
				else
					a = u, fa = fu;
				if (fu <= fpm || premin == x)
				{
					last_premin = premin, premin = u;
					flpm = fpm, fpm = fu;
				}
				else if (fu <= flpm || last_premin == x || last_premin == premin)
				{
					last_premin = u;
					flpm        = fu;
				}
			}

			co_yield {{a, fa}, {b, fb}};
		}
	}
};
