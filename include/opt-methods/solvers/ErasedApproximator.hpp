#pragma once

#include "./Approximator.hpp"

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
		virtual BoundsWithValues<P, V> operator()(std::function<V(P const&)> func, BoundsWithValues<P, V> const& bounds) = 0;
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
		constexpr auto hasNameHelper()
		{}
	public:
		template<typename ...Args>
		ErasedApproximatorImplementation(Args&&... a)
		: approx(std::forward<Args>(a)...)
		{}
		BoundsWithValues<P, V> operator()(std::function<V(P const&)> func, BoundsWithValues<P, V> const& bounds) override
		{
			return approx(func, bounds);
		}
		const char* name() const noexcept override
		{
			if constexpr (hasName<Approx>)
				return approx.name();
			else
				return "<erased unknown>";
		}
	};
}

template<typename T>
struct TypeTag {};
template<typename T>
constexpr TypeTag<T> typeTag;

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

	BoundsWithValues<P, V> operator()(std::function<V(P const&)> func, BoundsWithValues<P, V> const& bounds)
	{
		assert(holder != nullptr);
		return (*holder)(func, bounds);
	}

	char const* name() const noexcept
	{
		assert(holder != nullptr);
		return holder->name();
	}
};

