#pragma once

#include "./Approximator.hpp"
#include "./function/ErasedFunction.hpp"

#include <functional>
#include <type_traits>
#include <memory>
#include <cassert>

namespace impl
{
	template<typename P, typename V>
	class ErasedApproximator
	{
	public:
		virtual ApproxGenerator<P, V> operator()(ErasedFunction<V(P const&)> func, PointRegion<P> const& bounds) = 0;
		virtual void draw(BoundsWithValues<P, V> const& bounds, BaseIterationData<P, V> const& data, QtCharts::QChart& chart) = 0;
		virtual ~ErasedApproximator() {}
		virtual const char* name() const noexcept = 0;
	};

	template<typename T>
	concept hasName = requires(T const& t) {
		{ t.name() } noexcept;
		{ t.name() } -> std::same_as<char const*>;
	};

	template<typename P, typename V, Approximator<P, V> Approx>
	class ErasedApproximatorImplementation : public ErasedApproximator<P, V>
	{
	private:
		Approx approx;

	public:
		template<typename ...Args>
		ErasedApproximatorImplementation(Args&&... a)
		: approx(std::forward<Args>(a)...)
		{}
		ApproxGenerator<P, V> operator()(ErasedFunction<V(P const&)> func, PointRegion<P> const& bounds) override
		{
			return approx(std::move(func), bounds);
		}
		void draw(BoundsWithValues<P, V> const& bounds, BaseIterationData<P, V> const& data, QtCharts::QChart& chart) override
		{
			return approx.draw(bounds, data, chart);
		}
		const char* name() const noexcept override
		{
			if constexpr (hasName<Approx>)
				return approx.name();
			else
				return "<erased unknown>";
		}
	};

	template<typename T, typename P, typename V>
	concept FunctionHelper = requires(T const& f, P const& v) {
		{ f(v) } -> std::same_as<V>;
	};
}

template<typename T>
struct TypeTag {};
template<typename T>
constexpr TypeTag<T> typeTag;

/**
 * type-erasure for approximator to simply choose in UI without recompilation
 */
template<typename F, typename T>
class ErasedApproximator
{
public:
	using P = F;
	using V = T;
private:
	using BaseT = impl::ErasedApproximator<P, V>;
	std::unique_ptr<BaseT> holder;
 public:

	template<Approximator<P, V> Approx, typename ...Args>
	ErasedApproximator(TypeTag<Approx>, Args&&... a)
	: holder(new impl::ErasedApproximatorImplementation<P, V, Approx>(std::forward<Args>(a)...))
	{}

	ApproxGenerator<P, V> operator()(ErasedFunction<V(P const&)> func, PointRegion<P> bounds) const
	{
		assert(holder != nullptr);
		return (*holder)(std::move(func), std::move(bounds));
	}

	void draw(BoundsWithValues<P, V> bounds, BaseIterationData<P, V> const& data, QtCharts::QChart& chart)
	{
		assert(holder != nullptr);
		return holder->draw(std::move(bounds), data, chart);
	}

	char const* name() const noexcept
	{
		assert(holder != nullptr);
		return holder->name();
	}
};

