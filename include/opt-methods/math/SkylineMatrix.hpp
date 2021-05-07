#pragma once

#include "./Vector.hpp"
#include "./Matrix.hpp"

#include <iostream>
#include <tuple>
#include <vector>

template<typename T>
class SkylineMatrix
{
private:
	static const inline T zero{};

	/// diagonal storage
	std::vector<T> di;
	/// lower triangle storage (by rows)
	std::vector<T> al;
	/// upper triangle storage (by columns)
	std::vector<T> au;
	/// skyline information (ia[k] -- start of k-th row (column) in al (au))
	std::vector<int> ia = {0};

	int skylineStart(int i) const noexcept
	{
		return i - (ia[i + 1] - ia[i]);
	}

	template<auto data>
	friend struct LineProxy;

	template<auto data>
	struct LineProxy
	{
		friend class SkylineMatrix;

	private:
		SkylineMatrix const* const enclosing;
		int index, start;

		LineProxy(const SkylineMatrix* enclosing, int index) noexcept
		: enclosing(enclosing)
		, index(index)
		, start(enclosing->skylineStart(index))
		{}

		T const& getLineElement(int index, int dOffset) const noexcept
		{
			return (enclosing->*data)[enclosing->ia[index] + dOffset];
		}
		T const& getPerpElement(int offset, int dIndex) const noexcept
		{
			constexpr auto swapped = data == &SkylineMatrix::au ? &SkylineMatrix::al : &SkylineMatrix::au;
			return (enclosing->*swapped)[enclosing->ia[offset] + dIndex];
		}

		struct Iterator
		{
			friend struct LineProxy;
		private:
			LineProxy const* line;
			int offset;

			Iterator(LineProxy const* line, int offset) noexcept
			: line(line)
			, offset(offset)
			{}

		public:
			T const& operator*() const noexcept
			{
				return (*line)[offset];
			}
			T const* operator->() const noexcept
			{
				return &operator*();
			}
			Iterator& operator++() noexcept
			{
				++offset;
				return *this;
			}
			Iterator operator++(int) noexcept
			{
				return Iterator(line, offset++);
			}

			bool operator==(const Iterator &rhs) const = default;
			bool operator!=(const Iterator &rhs) const = default;
		};

	public:
		T const& operator[](int offset) const noexcept
		{
			assert(offset >= 0 && offset < enclosing->dims());
			if (int dOffset = offset - start; dOffset < 0)
				return zero;
			else if (offset < index) [[likely]]
				return getLineElement(index, dOffset);
			else if (offset == index)
				return enclosing->di[index];
			else [[unlikely]] if (int dIndex = index - enclosing->skylineStart(offset); dIndex < 0)
				return zero;
			else
				return getPerpElement(offset, dIndex);
		}

		Iterator iteratorAt(int offset) const noexcept
		{
			return Iterator(this, offset);
		}
		Iterator begin() const noexcept
		{
			return iteratorAt(0);
		}
		Iterator end() const noexcept
		{
			return iteratorAt(enclosing->dims());
		}
	};

public:
	SkylineMatrix() = default;

	int dims() const noexcept
	{
		return (int)ia.size() - 1;
	}
	auto row(int y) const& noexcept
	{
		assert(y >= 0 && y < dims());
		return LineProxy<&SkylineMatrix::al>(this, y);
	}
	auto col(int x) const& noexcept
	{
		assert(x >= 0 && x < dims());
		return LineProxy<&SkylineMatrix::au>(this, x);
	}
	auto operator[](int y) const& noexcept
	{
		return row(y);
	}

	SkylineMatrix&& LU() &&;

	Vector<T> solveSystem(const Vector<T>& b) &&;

	operator DenseMatrix<T>() const
	{
		int n = dims();
		std::valarray<T> data(zero, n * n);
		std::copy_n(di.begin(), n, util::StridedIterator(std::begin(data), n + 1));
		for (int i = 0; i < n; i++)
		{
			std::copy(al.data() + ia[i], al.data() + ia[i + 1], std::begin(data) + n * i + skylineStart(i));
			std::copy(au.begin() + ia[i],
			          au.begin() + ia[i + 1],
			          util::StridedIterator(std::begin(data), n, skylineStart(i), i, n));
		}
		return DenseMatrix<T>(n, data);
	}

private:
	void ReadFromHelp(std::istream& iIa, std::istream& iDi, std::istream& iAl, std::istream& iAu)
	{
		ia.clear();
		di.clear();
		al.clear();
		au.clear();

		util::ReadVector(iIa, ia);
		util::ReadVector(iDi, di);
		util::ReadVector(iAl, al);
		util::ReadVector(iAu, au);
	}

public:
	static SkylineMatrix ReadFrom(std::istream& iIa, std::istream& iDi, std::istream& iAl, std::istream& iAu)
	{
		SkylineMatrix ret;
		ret.ReadFromHelp(iIa, iDi, iAl, iAu);
		return ret;
	}

	void WriteTo(std::ostream& oIa, std::ostream& oDi, std::ostream& oAl, std::ostream& oAu) const
	{
		util::WriteVector(oIa, ia) << '\n';
		util::WriteVector(oDi, di) << '\n';
		util::WriteVector(oAl, al) << '\n';
		util::WriteVector(oAu, au) << '\n';
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

#include "LU.hpp"
