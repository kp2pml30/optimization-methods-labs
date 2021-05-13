#pragma once

#include <utility>
#include <exception>

#define METOPT_DEF_CONCAT_IMPL(s1, s2) s1 ## s2
#define METOPT_DEF_CONCAT(s1, s2) METOPT_DEF_CONCAT_IMPL(s1, s2)

#define METOPT_UNIQUE_NAME(pref) METOPT_DEF_CONCAT(pref, __LINE__)

namespace impl
{
	template<typename Func, typename CRTP>
	struct ConditionalScopeGuard
	{
	private:
		Func func;
	public:
		ConditionalScopeGuard(Func func)
		: func(std::move(func))
		{}

		~ConditionalScopeGuard() noexcept(noexcept(func()))
		{
			if (static_cast<CRTP&>(*this).NeedsRun())
				func();
		}
	};
	template<template<typename> typename Nested>
	struct ScopeGuardWorkaround
	{
		template<typename Func>
		Nested<Func> operator+(Func&& f)
		{
			return Nested<std::decay_t<Func>>{std::forward<Func>(f)};
		}
	};

	template<typename Func>
	struct ScopeGuard : ConditionalScopeGuard<Func, ScopeGuard<Func>>
	{
	private:
		using Base = ConditionalScopeGuard<Func, ScopeGuard<Func>>;

	public:
		using Base::Base;
		bool NeedsRun() noexcept { return true; }
	};


	template<typename Func, bool succ>
	struct ScopeGuardExcept : ConditionalScopeGuard<Func, ScopeGuardExcept<Func, succ>>
	{
	private:
		using Base = ConditionalScopeGuard<Func, ScopeGuardExcept<Func, succ>>;
		int initialExceptions;

	public:
		ScopeGuardExcept(Func&& f)
		: Base(std::forward<Func>(f))
		, initialExceptions(std::uncaught_exceptions())
		{}

		bool NeedsRun() noexcept
		{
			return (std::uncaught_exceptions() == initialExceptions) == succ;
		}
	};

	template<typename Func>
	using ScopeGuardSucc = ScopeGuardExcept<Func, true>;
	template<typename Func>
	using ScopeGuardFail = ScopeGuardExcept<Func, false>;
}

#define SCOPE_GUARD_IMPL(x) auto METOPT_UNIQUE_NAME(guard_) = impl::ScopeGuardWorkaround<x>{} + []() noexcept

#define SCOPE_GUARD_EX auto METOPT_UNIQUE_NAME(guard_) = impl::ScopeGuardWorkaround<impl::ScopeGuard>{} +
#define SCOPE_GUARD SCOPE_GUARD_IMPL(impl::ScopeGuard)
#define SCOPE_SUCCESS SCOPE_GUARD_IMPL(impl::ScopeGuardSucc)
#define SCOPE_FAIL SCOPE_GUARD_IMPL(impl::ScopeGuardFail)
