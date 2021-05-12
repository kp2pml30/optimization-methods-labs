#pragma once

#include "./Vector.hpp"
#include "./Matrix.hpp"
#include "../util/Util.hpp"

#include <iostream>
#include <tuple>
#include <vector>
#include <functional>
#include <filesystem>
#include <fstream>

template<typename T>
class DenseMatrix;

template<typename T>
class SkylineMatrix;

namespace util
{
	struct SkylineMatrixGeneratorImpl;
}

template<typename T>
Vector<T> operator*(SkylineMatrix<T> const& m, Vector<T> const& x);

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

	int SkylineStart(int i) const noexcept
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
		, start(enclosing->SkylineStart(index))
		{}

		T const& GetLineElement(int index, int dOffset) const noexcept
		{
			return (enclosing->*data)[enclosing->ia[index] + dOffset];
		}
		T const& GetPerpElement(int offset, int dIndex) const noexcept
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

			bool operator==(const Iterator& rhs) const = default;
			bool operator!=(const Iterator& rhs) const = default;
		};

	public:
		T const& operator[](int offset) const noexcept
		{
			assert(offset >= 0 && offset < enclosing->Dims());
			if (int dOffset = offset - start; dOffset < 0)
				return util::zero<T>;
			else if (offset < index) [[likely]]
				return GetLineElement(index, dOffset);
			else if (offset == index)
				return enclosing->di[index];
			else [[unlikely]] if (int dIndex = index - enclosing->SkylineStart(offset); dIndex < 0)
				return util::zero<T>;
			else
				return GetPerpElement(offset, dIndex);
		}

		Iterator IteratorAt(int offset) const noexcept
		{
			return Iterator(this, offset);
		}
		Iterator begin() const noexcept
		{
			return IteratorAt(0);
		}
		Iterator end() const noexcept
		{
			return IteratorAt(enclosing->Dims());
		}
	};

public:
	SkylineMatrix() = default;

	int Dims() const noexcept
	{
		return (int)ia.size() - 1;
	}
	auto Row(int y) const& noexcept
	{
		assert(y >= 0 && y < Dims());
		return LineProxy<&SkylineMatrix::al>(this, y);
	}
	auto Col(int x) const& noexcept
	{
		assert(x >= 0 && x < Dims());
		return LineProxy<&SkylineMatrix::au>(this, x);
	}
	auto operator[](int y) const& noexcept
	{
		return Row(y);
	}

	SkylineMatrix&& LU() &&;

	Vector<T> SolveSystem(const Vector<T>& b) &&;

	auto ExtractData() &&
	{
		return std::make_tuple(std::move(ia), std::move(di), std::move(al), std::move(au));
	}

	operator DenseMatrix<T>() const
	{
		int n = Dims();
		std::valarray<T> data(util::zero<T>, n * n);
		std::copy_n(di.begin(), n, util::StridedIterator(std::begin(data), n + 1));
		for (int i = 0; i < n; i++)
		{
			std::copy(al.data() + ia[i], al.data() + ia[i + 1], std::begin(data) + n * i + SkylineStart(i));
			std::copy(au.begin() + ia[i],
			          au.begin() + ia[i + 1],
			          util::StridedIterator(std::begin(data), n, SkylineStart(i), i, n));
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

		using namespace util;
		iIa >> ia;
		iDi >> di;
		iAl >> al;
		iAu >> au;
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
		using namespace util;
		oIa << ia << '\n';
		oDi << di << '\n';
		oAl << al << '\n';
		oAu << au << '\n';
	}

	static SkylineMatrix ReadFrom(std::filesystem::path const& p)
	{
		SkylineMatrix ret;
		auto ia = std::ifstream(p / "ia.txt"), di = std::ifstream(p / "di.txt"), al = std::ifstream(p / "al.txt"),
		     au = std::ifstream(p / "au.txt");
		ret.ReadFromHelp(ia, di, al, au);
		return ret;
	}

	void WriteTo(std::filesystem::path const& p) const
	{
		auto ia = std::ofstream(p / "ia.txt"), di = std::ofstream(p / "di.txt"), al = std::ofstream(p / "al.txt"),
		     au = std::ofstream(p / "au.txt");
		WriteTo(ia, di, al, au);
	}

	friend Vector<T> operator*<T>(SkylineMatrix<T> const& m, Vector<T> const& x);
	friend struct util::SkylineMatrixGeneratorImpl;
};

namespace util
{
	struct SkylineMatrixGeneratorImpl
	{
		template<typename T>
		static SkylineMatrix<T>& GenerateM(
		    MatrixGenerator<T, SkylineMatrix<T>> const& gen,
		    SkylineMatrix<T>& m,
		    size_t n,
		    std::vector<ptrdiff_t> const& selectedDiagonals,
		    std::invocable<std::default_random_engine&, size_t, size_t> auto&& aijGenerator,
		    std::invocable<std::default_random_engine&, size_t> auto&& diGenerator)
		{
			m.ia.resize(n + 1);
			m.ia[0]         = 0;
			auto [min, max] = std::minmax_element(selectedDiagonals.begin(), selectedDiagonals.end());
			int skyline     = (int)std::max(std::abs(*min), std::abs(*max));
			for (int i = 1, val = 0; i < (int)n + 1; i++, val += skyline)
				m.ia[i] = m.ia[i - 1] + std::min(val, i - 1);
			m.al.resize(m.ia.back());
			m.au.resize(m.ia.back());
			m.di.resize(n);

			for (auto diag : selectedDiagonals)
			{
				if (diag > 0)
					for (int i = (int)diag; i < (int)n; i++)
						m.al[m.ia[i + 1] - diag] = aijGenerator(gen.engine, i, i - diag);
				else
					for (int i = (int)(-diag); i < (int)n; i++)
						m.au[m.ia[i + 1] + diag] = aijGenerator(gen.engine, i + diag, i);
			}
			for (int i = 0; i < (int)n; i++)
				m.di[i] = diGenerator(gen.engine, i);
			return m;
		}

		template<typename T>
		static SkylineMatrix<T> DiagonallyDominant(MatrixGenerator<T, SkylineMatrix<T>> const& gen,
		                                           size_t n,
		                                           T dominance,
		                                           std::vector<ptrdiff_t> const& selectedDiagonals,
		                                           std::invocable<std::default_random_engine&> auto&& aijDistribution)
		{
			SkylineMatrix<T> m;
			return GenerateM<T>(
			    gen,
			    m,
			    n,
			    selectedDiagonals,
			    [&](auto& gen, size_t, size_t) { return aijDistribution(gen); },
			    [&, isFirst = true, sum = util::zero<T>](auto&, size_t) mutable {
				    if (isFirst)
				    {
					    sum     = std::reduce(m.al.begin(), m.al.end()) + std::reduce(m.au.begin(), m.au.end());
					    isFirst = false;
					    return -sum + dominance;
				    }
				    return -sum;
			    });
		}

		template<typename T>
		static SkylineMatrix<T> Hilbert(MatrixGenerator<T, SkylineMatrix<T>> const& gen,
		                                size_t n,
		                                std::vector<ptrdiff_t> const& selectedDiagonals)
		{
			SkylineMatrix<T> m;
			return GenerateM<T>(
			    gen,
			    m,
			    n,
			    selectedDiagonals,
			    [&](auto&, size_t i, size_t j) { return T{1} / (i + j + 1); },
			    [&](auto&, size_t i) mutable { return T{1} / (i + i + 1); });
		}
	};

	template<typename T>
	SkylineMatrix<T> DiagonallyDominant(MatrixGenerator<T, SkylineMatrix<T>> const& gen,
	                                    size_t n,
	                                    T dominance,
	                                    std::vector<ptrdiff_t> const& selectedDiagonals,
	                                    std::invocable<std::default_random_engine&> auto&& aijDistribution)
	{
		return SkylineMatrixGeneratorImpl::DiagonallyDominant<T>(
		    gen, n, dominance, selectedDiagonals, std::forward<decltype(aijDistribution)>(aijDistribution));
	}

	template<typename T>
	SkylineMatrix<T> Hilbert(MatrixGenerator<T, SkylineMatrix<T>> const& gen,
	                         size_t n,
	                         std::vector<ptrdiff_t> const& selectedDiagonals)
	{
		return SkylineMatrixGeneratorImpl::Hilbert<T>(
		    gen, n, selectedDiagonals);
	}
} // namespace util

template<typename T>
std::ostream& PrintDense(std::ostream& o, SkylineMatrix<T> const& m)
{
	o << m.Dims() << '\n';
	for (int y = 0; y < m.Dims(); y++)
	{
		if (m.Dims() > 0)
			o << m[y][0];
		for (int x = 1; x < m.Dims(); x++)
			o << '\t' << m[y][x];
		o << '\n';
	}
	return o;
}

template<typename T>
Vector<T> operator*(SkylineMatrix<T> const& m, Vector<T> const& x)
{
	assert(m.Dims() == (int)x.size());
	Vector<T> y(m.Dims());
	for (int i = 0; i < m.Dims(); i++)
	{
		y[i] += m.di[i] * x[i];
		y[i] += std::transform_reduce(m.al.begin() + m.ia[i], m.al.begin() + m.ia[i + 1], std::begin(x) + m.SkylineStart(i), T{});
		std::transform(m.au.begin() + m.ia[i],
		               m.au.begin() + m.ia[i + 1],
		               std::begin(y) + m.SkylineStart(i),
		               std::begin(y) + m.SkylineStart(i),
		               [fac = x[i]](T const& lhs, T const& rhs) { return rhs + fac * lhs; });
	}
	return y;
}

#include "LU.hpp"
