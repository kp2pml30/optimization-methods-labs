#pragma once

#include <utility>
#include <unordered_map>
#include <string>

template<typename T, typename Stat>
struct CountedFloat
{
	T data;

	CountedFloat(T data = 0)
	: data(std::move(data))
	{}
#define MAKEBOP(op) \
	friend CountedFloat operator op(CountedFloat const& a, CountedFloat const& b) \
	{ \
		Stat{}(#op); \
		return CountedFloat(a.data op b.data); \
	} \
	friend CountedFloat& operator op##=(CountedFloat& a, CountedFloat const& b) \
	{ \
		Stat{}(#op); \
		a.data op##= b.data; \
		return a;\
	}
	MAKEBOP(+)
	MAKEBOP(-)
	MAKEBOP(*)
	MAKEBOP(/)
#undef MAKEBOP
#define MAKEUOP(op) \
	friend CountedFloat operator op(CountedFloat const& a) \
	{ \
		return CountedFloat(op a.data); \
	}
	MAKEUOP(+)
	MAKEUOP(-)
#undef MAKEUOP

	friend auto operator<=>(CountedFloat const& l ,CountedFloat const& r)
	{
		Stat{}("<=>");
		return l.data <=> r.data;
	}

	friend CountedFloat abs(CountedFloat const& c)
	{
		if (c < 0)
			return -c;
		return c;
	}

	friend auto& operator<<(std::ostream& o, CountedFloat const& a)
	{
		return o << a.data;
	}
	friend auto& operator>>(std::istream& o, CountedFloat& a)
	{
		return o >> a.data;
	}
};
