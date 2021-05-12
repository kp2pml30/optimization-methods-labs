#pragma once

#include "./Vector.hpp"
#include "./Matrix.hpp"
#include "./SkylineMatrix.hpp"
#include "../util/Util.hpp"

#include <iostream>
#include <tuple>
#include <vector>
#include <functional>
#include <filesystem>
#include <fstream>

template<typename T>
class RowColumnSymMatrix;

namespace util
{
	struct RowColumnSymMatrixGeneratorImpl;
}

template<typename T>
Vector<T> operator*(RowColumnSymMatrix<T> const& m, Vector<T> const& x);

template<typename T>
class RowColumnSymMatrix
{
private:
	/// diagonal storage
	std::vector<T> di;
	/// lower triangle (upper triangle) storage by rows (by columns)
	std::vector<T> al;
	/// mapping j -> al[j]'s column index
	std::vector<int> ja;
	/// mapping k -> start of k-th row in al and ja
	std::vector<int> ia = {0};

	int SkylineStart(int i) const noexcept
	{
		return ja[ia[i]];
	}

public:
	RowColumnSymMatrix() = default;

	int Dims() const noexcept
	{
		return (int)ia.size() - 1;
	}

	Vector<T> SolveSystem(const Vector<T>& b, T epsilon = 1e-7, int* outNIters = nullptr);

	operator DenseMatrix<T>() const
	{
		int n = Dims();
		std::valarray<T> data(util::zero<T>, n * n);
		std::copy_n(di.begin(), n, util::StridedIterator(std::begin(data), n + 1));
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < ia[i + 1] - ia[i]; j++)
			{
				int r = i, c = ja[ia[i] + j];
				data[c * n + r] = data[r * n + c] = al[ia[i] + j];
			}
			data[i * n + i] = di[i];
		}
		return DenseMatrix<T>(n, data);
	}

private:
	void ReadFromHelp(std::istream& iIa, std::istream& iJa, std::istream& iDi, std::istream& iAl)
	{
		ia.clear();
		ja.clear();
		di.clear();
		al.clear();

		using namespace util;
		iIa >> ia;
		iJa >> ja;
		iDi >> di;
		iAl >> al;
	}

public:
	static RowColumnSymMatrix ReadFrom(std::istream& iIa, std::istream& iJa, std::istream& iDi, std::istream& iAl)
	{
		RowColumnSymMatrix ret;
		ret.ReadFromHelp(iIa, iJa, iDi, iAl);
		return ret;
	}

	void WriteTo(std::ostream& oIa, std::ostream& oJa, std::ostream& oDi, std::ostream& oAl) const
	{
		using namespace util;
		oIa << ia << '\n';
		oJa << ja << '\n';
		oDi << di << '\n';
		oAl << al << '\n';
	}

	static RowColumnSymMatrix ReadFrom(std::filesystem::path const& p)
	{
		RowColumnSymMatrix ret;
		auto ia = std::ifstream(p / "ia.txt"), ja = std::ifstream(p / "ja.txt"), di = std::ifstream(p / "di.txt"),
		     al = std::ifstream(p / "al.txt");
		ret.ReadFromHelp(ia, ja, di, al);
		return ret;
	}

	void WriteTo(std::filesystem::path const& p) const
	{
		auto ia = std::ofstream(p / "ia.txt"), ja = std::ofstream(p / "ja.txt"), di = std::ofstream(p / "di.txt"),
		     al = std::ofstream(p / "al.txt");
		WriteTo(ia, ja, di, al);
	}

	friend Vector<T> operator*<T>(RowColumnSymMatrix<T> const& m, Vector<T> const& x);
	friend struct util::RowColumnSymMatrixGeneratorImpl;
};

namespace util
{
	struct RowColumnSymMatrixGeneratorImpl
	{
		template<typename T>
		static RowColumnSymMatrix<T>& GenerateM(
		    MatrixGenerator<T, RowColumnSymMatrix<T>> const& gen,
		    RowColumnSymMatrix<T>& m,
		    size_t n,
		    std::vector<ptrdiff_t> const& selectedDiagonals,
		    std::invocable<std::default_random_engine&, size_t, size_t> auto&& aijGenerator,
		    std::invocable<std::default_random_engine&, size_t> auto&& diGenerator)
		{
			SkylineMatrix<T> temp;
			util::SkylineMatrixGeneratorImpl::GenerateM(static_cast<MatrixGenerator<T, SkylineMatrix<T>>>(gen),
			                                            temp,
			                                            n,
			                                            selectedDiagonals,
			                                            std::forward<decltype(aijGenerator)>(aijGenerator),
			                                            [](auto&, size_t) { return util::zero<T>; });
			auto [ia, di, al, au] = std::move(temp).ExtractData();
			m.ia = std::move(ia);
			m.al.resize(al.size());
			m.ja.resize(al.size());
			std::transform(
			    al.begin(), al.end(), au.begin(), m.al.begin(), [](T const& lhs, T const& rhs) { return (lhs + rhs) / 2; });
			for (int i = 0; i < (int)n; i++)
				for (int skylineLen = m.ia[i + 1] - m.ia[i], skylineStart = i - skylineLen, j = 0; j < skylineLen; j++)
					m.ja[m.ia[i] + j] = skylineStart + j;
			m.di.resize(n);
			for (int i = 0; i < (int)n; i++)
				m.di[i] = diGenerator(gen.engine, i);
			return m;
		}

		template<typename T>
		static RowColumnSymMatrix<T> DiagonallyDominant(MatrixGenerator<T, RowColumnSymMatrix<T>> const& gen,
		                                                size_t n,
		                                                T dominance,
		                                                std::vector<ptrdiff_t> const& selectedDiagonals,
		                                                std::invocable<std::default_random_engine&> auto&& aijDistribution)
		{
			RowColumnSymMatrix<T> m;
			return GenerateM<T>(
			    gen,
			    m,
			    n,
			    selectedDiagonals,
			    [&](auto& gen, size_t, size_t) { return aijDistribution(gen); },
			    [&, isFirst = true, sum = util::zero<T>](auto&, size_t) mutable {
				    if (isFirst)
				    {
					    sum     = 2 * std::reduce(m.al.begin(), m.al.end());
					    isFirst = false;
					    return -sum + dominance;
				    }
				    return -sum;
			    });
		}

		template<typename T>
		static RowColumnSymMatrix<T> Hilbert(MatrixGenerator<T, RowColumnSymMatrix<T>> const& gen,
		                                     size_t n,
		                                     std::vector<ptrdiff_t> const& selectedDiagonals)
		{
			RowColumnSymMatrix<T> m;
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
	RowColumnSymMatrix<T> DiagonallyDominant(MatrixGenerator<T, RowColumnSymMatrix<T>> const& gen,
	                                         size_t n,
	                                         T dominance,
	                                         std::vector<ptrdiff_t> const& selectedDiagonals,
	                                         std::invocable<std::default_random_engine&> auto&& aijDistribution)
	{
		return RowColumnSymMatrixGeneratorImpl::DiagonallyDominant<T>(
		    gen, n, dominance, selectedDiagonals, std::forward<decltype(aijDistribution)>(aijDistribution));
	}

	template<typename T>
	RowColumnSymMatrix<T> Hilbert(MatrixGenerator<T, RowColumnSymMatrix<T>> const& gen,
	                              size_t n,
	                              std::vector<ptrdiff_t> const& selectedDiagonals)
	{
		return RowColumnSymMatrixGeneratorImpl::Hilbert<T>(
		    gen, n, selectedDiagonals);
	}
} // namespace util

template<typename T>
Vector<T> operator*(RowColumnSymMatrix<T> const& m, Vector<T> const& x)
{
	assert(m.Dims() == (int)x.size());
	Vector<T> y(m.Dims());
	for (int r = 0; r < m.Dims(); r++)
	{
		y[r] += m.di[r] * x[r];
		for (int j = 0; j < m.ia[r + 1] - m.ia[r]; j++)
		{
			int c = m.ja[m.ia[r] + j];

			y[r] += m.al[m.ia[r] + j] * x[c];
			y[c] += m.al[m.ia[r] + j] * x[r];
		}
	}
	return y;
}

template<typename T>
Vector<T> RowColumnSymMatrix<T>::SolveSystem(const Vector<T>& b, T epsilon, int* outNIters)
{
	assert(Dims() == (int)b.size());
	Vector<T>
		x(util::zero<T>, b.size()),
		r = b - *this * x,
		z = r;
	T r2 = Dot(r, r), len2b = Len2(b), eps2 = epsilon * epsilon;

	int nIters = 0;
	do
	{
		Vector<T> Az = *this * z;
		T alpha = r2 / Dot(Az, z);
		x += alpha * z;
		r -= alpha * Az;
		T new_r2 = Dot(r, r), beta = new_r2 / r2;
		z = r + beta * z;
		r2 = new_r2;
	} while (++nIters <= 1000 * Dims() && Len2(r) / len2b >= eps2);
	if (outNIters != nullptr)
		*outNIters = nIters;
	return x;
}
