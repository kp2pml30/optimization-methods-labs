#pragma once

#include <utility>
#include <map>
#include <string>

template<typename T>
struct CountedFloat
{
	T data;
	static inline std::map<std::string, std::size_t> stats;

	CountedFloat(T data)
	: data(std::move(data))
	{}
#define MAKEBOP(op) \
	friend CountedFloat operator op(CountedFloat const& a, CountedFloat const& b) \
	{ \
		stats[#op]++; \
		return CountedFloat(a.data op b.data); \
	} \
	friend CountedFloat& operator op##=(CountedFloat& a, CountedFloat const& b) \
	{ \
		stats[#op]++; \
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
};
