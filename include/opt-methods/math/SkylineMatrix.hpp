#pragma once

#include "./ShiftedView.hpp"

#include <vector>
#include <tuple>
#include <iostream>

template<typename T>
class SkylineMatrix
{
private:
	/// diagonal storage
	std::vector<T> di;
	/// lower triangle storage (by rows)
	std::vector<T> al;
	/// upper triangle storage (by columns)
	std::vector<T> au;
	/// skyline information (ia[k] -- start of k-th row (column) in al (au))
	std::vector<int> ia = {0};

	int skylineStart(int i) const noexcept {
		return i - (ia[i + 1] - ia[i]);
	}

	friend struct RowProxy;
	struct RowProxy {
		friend class SkylineMatrix;
	private:
		const SkylineMatrix* const enclosing;
		int y, start;

		RowProxy(const SkylineMatrix* enclosing, int y) noexcept
		: enclosing(enclosing)
		, y(y)
		, start(enclosing->skylineStart(y))
		{}

		T getRowElement(int y, int dx) const noexcept {
			return enclosing->al[enclosing->ia[y] + dx];
		}
		T getColElement(int x, int dy) const noexcept {
			return enclosing->au[enclosing->ia[x] + dy];
		}

	public:
		T operator[](int x) const noexcept {
			assert(x >= 0 && x < enclosing->dims());
			if (int dx = x - start; dx < 0)
				return {};
			else if (x < y) [[likely]]
				return getRowElement(y, dx);
			else if (x == y)
				return enclosing->di[y];
			else [[unlikely]]
				if (int dy = y - enclosing->skylineStart(x); dy < 0)
					return {};
				else
					return getColElement(x, dy);
		}
	};

public:
	SkylineMatrix() = default;

	int dims() const noexcept
	{
		return (int)ia.size() - 1;
	}
	auto operator[](int y) const noexcept
	{
		assert(y >= 0 && y < dims());
		return RowProxy(this, y);
	}

private:
	template<typename TT>
	static std::ostream& writeVector(std::ostream& o, const std::vector<TT>& v) {
		for (auto &el : v)
			o << el << ' ';
		return o;
	}

	template<typename TT>
	static void readVector(std::istream& i, std::vector<TT>& v) {
		for (auto &el : v)
			i >> el;
	}

	void ReadFromHelp(std::istream& i)
	{
		int n;
		i >> n;

		ia.resize(n + 1);
		readVector(i, ia);

		di.resize(n);
		al.resize(ia.back());
		au.resize(ia.back());

		readVector(i, di);
		readVector(i, al);
		readVector(i, au);
	}

	void WriteTo(std::ostream& o) const
	{
		int n = dims();
		o << n << '\n';
		writeVector(o, ia) << '\n';
		writeVector(o, di) << '\n';
		writeVector(o, al) << '\n';
		writeVector(o, au);
	}
public:
	static SkylineMatrix ReadFrom(std::istream& i)
	{
		SkylineMatrix ret;
		ret.ReadFromHelp(i);
		return ret;
	}
	friend std::istream& operator>>(std::istream& i, SkylineMatrix m)
	{
		m.ReadFromHelp(i);
		return i;
	}
	friend std::ostream& operator<<(std::ostream& o, SkylineMatrix const& m)
	{
		m.WriteTo(o);
		return o;
	}
};

template<typename T>
std::ostream& PrintDense(std::ostream& o, SkylineMatrix<T> const& m)
{
	o << m.dims() << '\n';
	for (int y = 0; y < m.dims(); y++)
	{
		if (m.dims() > 0)
			o << m[y][0];
		for (int x = 1; x < m.dims(); x++)
			o << '\t' << m[y][x];
		o << '\n';
	}
	return o;
}
