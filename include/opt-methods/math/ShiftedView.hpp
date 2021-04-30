#pragma once

#include <cassert>
#include <span>

template<typename T>
class ShiftedView
{
private:
	std::span<const T> view;
	int start;
	int end;
	T zero{};
public:
	ShiftedView(std::span<const T> view, int start, int end) noexcept
	: view(std::move(view))
	, start(start)
	, end(end)
	{}

	T const& operator[](int index) const noexcept
	{
		assert(index >= 0 && index < end);
		int ni = index - start;
		if (ni < 0 || ni >= (int)view.size())
			return zero;
		return view[ni];
	}
};
