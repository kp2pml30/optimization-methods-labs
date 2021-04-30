#pragma once

#include "./ShiftedView.hpp"

#include <vector>
#include <tuple>
#include <iostream>

template<typename T>
class ProfileMatrix
{
private:
	/// storage
	std::vector<T> data;
	/// index in row where starts
	std::vector<int> starts;
	/// length of profile
	std::vector<int> lens;
	/// index in data where starts, helper
	std::vector<int> starts_data;
	int w_, h_;
public:
	ProfileMatrix() = default;

	int w() const noexcept
	{
		return w_;
	}
	int h() const noexcept
	{
		return w_;
	}
	ShiftedView<T> operator[](int y) const noexcept
	{
		assert(y >= 0 && y < h());
		return ShiftedView<T>(std::span<const T>(&data[starts_data[y]], lens[y]), starts[y], w());
	}

	/**
	 * @return data, starts, lens, starts_data as references
	 */
	std::tuple<std::vector<T>&, std::vector<int>&, std::vector<int>&, std::vector<int>&> Fetch() noexcept
	{
		return {data, starts, lens, starts_data};
	}

private:
	void ReadFromHelp(std::istream& i)
	{
		i >> h_ >> w_;
		starts.reserve(h_);
		starts_data.reserve(h_);
		lens.reserve(h_);
		for (int y = 0; y < h_; y++)
		{
			int s, l;
			i >> s >> l;
			data.reserve(l);
			starts_data.emplace_back(data.size());
			starts.emplace_back(s);
			lens.emplace_back(l);
			for (int x = 0; x < l; x++)
			{
				T p;
				i >> p;
				data.emplace_back(std::move(p));
			}
		}
	}

	void WriteTo(std::ostream& o) const
	{
		o << h() << ' ' << w() << '\n';
		for (int y = 0; y < h(); y++)
		{
			o << starts[y] << ' ' << lens[y];
			for (int x = 0; x < lens[y]; x++)
				o << '\t' << data[starts_data[y] + x];
			o << '\n';
		}
	}
public:
	static ProfileMatrix ReadFrom(std::istream& i)
	{
		ProfileMatrix ret;
		ret.ReadFromHelp(i);
		return ret;
	}
	friend std::istream& operator>>(std::istream& i, ProfileMatrix m)
	{
		m.ReadFromHelp(i);
		return i;
	}
	friend std::ostream& operator<<(std::ostream& o, ProfileMatrix const& m)
	{
		m.WriteTo(o);
		return o;
	}
};

template<typename T>
std::ostream& PrintDense(std::ostream& o, ProfileMatrix<T> const& m)
{
	o << m.h() << ' ' << m.w() << '\n';
	for (int y = 0; y < m.h(); y++)
	{
		if (m.w() > 0)
			o << m[y][0];
		for (int x = 1; x < m.w(); x++)
			o << '\t' << m[y][x];
		o << '\n';
	}
	return o;
}
