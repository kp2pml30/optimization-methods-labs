// this file is initially part of another project, codestyle does not match.
#pragma once

#include <exception>
#include <type_traits>

#include "opt-methods/math/Scalar.hpp"

template<typename F>
struct ErasedFunction;

template<typename R, typename ...Args>
struct GetGradType { using type = void; };

template<typename R, typename A>
struct GetGradType<R, A> { using type = ErasedFunction<ScalarSubst<std::decay_t<A>, std::decay_t<R>>(A)>; };

template<typename ...T>
using GetGradTypeT = typename GetGradType<T...>::type;

template<typename T>
concept HasGrad = requires(T const& t) {
	{ t.grad() };
};

class bad_function_call : public std::exception
{
public:
	using std::exception::exception;
};

namespace function_helper
{
	template<typename T, typename Dummy = void>
	struct FunctionTraits
	{};

	template<typename T>
	constexpr bool is_small_object_v = sizeof(T) <= sizeof(void*) && alignof(void*) % alignof(T) == 0
	                                   && std::is_nothrow_constructible_v<T, T&&>;

	using FunctionBuffer = std::aligned_storage_t<sizeof(void*), alignof(void*)>;

	template<typename R, typename... Args>
	struct TypeDescriptor;

	template<typename R, typename... Args>
	struct FunctionStorage
	{
	private:
		template<typename Y>
		friend struct ::ErasedFunction;

		template<typename Y, typename D>
		friend struct FunctionTraits;

		FunctionBuffer buf;
		TypeDescriptor<R, Args...> const* desc;

	public:
		FunctionStorage() noexcept = default;

		template<typename T>
		T* SmallCast() noexcept
		{
			return reinterpret_cast<T*>(&buf);
		}

		template<typename T>
		T const* SmallCast() const noexcept
		{
			return reinterpret_cast<const T*>(&buf);
		}

		template<typename T>
		T* BigCast() const noexcept
		{
			using Tptr = T*;
			return *reinterpret_cast<Tptr const*>(&buf);
		}
		template<typename T>
		T*& BigCast() noexcept
		{
			using Tptr = T*;
			return *reinterpret_cast<Tptr*>(&buf);
		}

		void Copy(FunctionStorage const& rhs)
		{
			decltype(buf) copy   = buf;
			decltype(desc) dcopy = desc;
			rhs.desc->Copy(this, &rhs);
			desc = std::move(dcopy);
			std::swap(copy, buf);
			desc->Destroy(this);
			buf  = copy;
			desc = rhs.desc;
		}
	};

	template<typename R, typename... Args>
	struct TypeDescriptor
	{
	private:
		using FunctionStorage = function_helper::FunctionStorage<R, Args...>;

	public:
		void (*Destroy)(FunctionStorage* del);
		void (*Move)(FunctionStorage* to, FunctionStorage* from);
		void (*Copy)(FunctionStorage* to, FunctionStorage const* from);
		R (*Invoke)(FunctionStorage const* what, Args...);
		GetGradTypeT<R, Args...> (*Grad)(FunctionStorage const* what);
	};

	template<typename R, typename... Args>
	TypeDescriptor<R, Args...> const* EmptyTypeDescriptor()
	{
		using FunctionStorage                           = function_helper::FunctionStorage<R, Args...>;
		constexpr static TypeDescriptor<R, Args...> ret = {
		    +[](FunctionStorage*) {},
		    +[](FunctionStorage* to, FunctionStorage* from) { *to = *from; },
		    +[](FunctionStorage* to, FunctionStorage const* from) { *to = *from; },
		    +[](FunctionStorage const*, Args...) -> R { throw bad_function_call(); },
		    +[](FunctionStorage const*) -> GetGradTypeT<R, Args...> { throw bad_function_call(); }};
		return &ret;
	}

	// small object
	template<typename T>
	struct FunctionTraits<T, std::enable_if_t<is_small_object_v<T>>>
	{
	public:
		template<typename R, typename... Args>
		static TypeDescriptor<R, Args...> const* Descr() noexcept
		{
			using FunctionStorage = function_helper::FunctionStorage<R, Args...>;
			constexpr static TypeDescriptor<R, Args...> ret{
			    +[](FunctionStorage* del) { del->template SmallCast<T>()->~T(); },
			    +[](FunctionStorage* to, FunctionStorage* from) {
				    new (&to->buf) T(std::move(*from->template SmallCast<T>()));
				    to->desc = from->desc;
			    },
			    +[](FunctionStorage* to, FunctionStorage const* from) {
				    new (&to->buf) T(*from->template SmallCast<T>());
				    to->desc = from->desc;
			    },
			    +[](FunctionStorage const* what, Args... args) -> R {
				    // here is a const problem with small object optimization
				    if constexpr (0)
					    return what->template SmallCast<T>()->operator()(std::forward<Args>(args)...);
				    else
					    return const_cast<T*>(what->template SmallCast<T>())->operator()(std::forward<Args>(args)...);
			    },
			    +[](FunctionStorage const* what) -> GetGradTypeT<R, Args...> {
				    if constexpr (HasGrad<T>)
					    return what->template SmallCast<T>()->grad();
				    else
					    throw bad_function_call();
			    }};
			return &ret;
		}
	};

	// big object
	template<typename T>
	struct FunctionTraits<T, std::enable_if_t<!is_small_object_v<T>>>
	{
		template<typename R, typename... Args>
		static TypeDescriptor<R, Args...> const* Descr() noexcept
		{
			using FunctionStorage                           = function_helper::FunctionStorage<R, Args...>;
			constexpr static TypeDescriptor<R, Args...> ret = {
			    +[](FunctionStorage* del) { delete del->template BigCast<T>(); },
			    +[](FunctionStorage* to, FunctionStorage* from) {
				    to->template BigCast<T>() = from->template BigCast<T>();
				    to->desc                  = from->desc;
				    from->desc                = EmptyTypeDescriptor<R, Args...>();
			    },
			    +[](FunctionStorage* to, FunctionStorage const* from) {
				    to->template BigCast<T>() = new T(*from->template BigCast<T>());
				    to->desc                  = from->desc;
			    },
			    +[](const FunctionStorage* what, Args... args) -> R {
				    return what->template BigCast<T>()->operator()(std::forward<Args>(args)...);
			    },
			    +[]([[maybe_unused]] const FunctionStorage* what) -> GetGradTypeT<R, Args...> {
				    if constexpr (HasGrad<T>)
					    return what->template BigCast<T>()->grad();
				    else
					    throw bad_function_call();
			    }};
			return &ret;
		}
	};
} // namespace function_helper
