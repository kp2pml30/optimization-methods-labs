#pragma once

#include <iterator>
#include <vector>
#include <span>
#include <random>

namespace util
{
	template<typename TT>
	static std::ostream& WriteVector(std::ostream& o, TT const& v)
	{
		for (auto& el : v)
			o << el << ' ';
		return o;
	}

	template<typename TT>
	static void ReadVector(std::istream& i, std::vector<TT>& v)
	{
		TT el{};
		while (i >> el)
			v.push_back(el);
	}

	template<typename It> requires std::forward_iterator<It>
	struct StridedIterator : std::iterator_traits<It>
	{
		It cur;
		size_t stride;

		StridedIterator() = default;
		StridedIterator(It iter, size_t stride) noexcept
		: cur(iter)
		, stride(stride)
		{}
		StridedIterator(It iter, size_t n, size_t startI, size_t startJ, size_t stride) noexcept
		: cur(iter + (startI * n + startJ))
		, stride(stride)
		{}
		StridedIterator& operator++() noexcept
		{
			std::advance(cur, stride);
			return *this;
		}
		StridedIterator operator++(int) noexcept
		{
			auto res = StridedIterator(*this);
			++(*this);
			return res;
		}

		auto* operator->() const
		{
			return cur.operator->();
		}
		auto& operator*() const
		{
			return *cur;
		}
		bool operator==(const StridedIterator& rhs)
		{
			return cur == rhs.cur;
		}
		bool operator!=(const StridedIterator& rhs)
		{
			return cur != rhs.cur;
		}

		operator It() const
		{
			return cur;
		}
	};

	template<typename It> requires std::forward_iterator<It>
	struct PermutedStridedIterator : StridedIterator<It>
	{
		std::span<int> permutation;
		size_t i = 0;

		PermutedStridedIterator() = default;
		PermutedStridedIterator(
		    It iter, size_t n, size_t startI, size_t startJ, size_t stride, std::span<int> permutation) noexcept
		: StridedIterator<It>(iter, n, permutation[startI], startJ, stride)
		, permutation(permutation)
		, i(startI)
		{}

		PermutedStridedIterator& operator++() noexcept
		{
			std::advance(this->cur, (permutation[i + 1] - permutation[i]) * this->stride);
			i++;
			return *this;
		}
		PermutedStridedIterator operator++(int) noexcept
		{
			auto res = PermutedStridedIterator(*this);
			++(*this);
			return res;
		}
	};
}
