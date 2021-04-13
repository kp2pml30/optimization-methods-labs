// this file is initially part of another project, codestyle does not match.
#pragma once

#include "./function-helper.hpp"

template<typename>
struct IsErasedFunction : public std::false_type {};
template<typename ...A>
struct IsErasedFunction<ErasedFunction<A...>> : public std::true_type {};

template<typename R, typename... Args>
struct ErasedFunction<R(Args...)>
{
private:
	function_helper::FunctionStorage<R, Args...> storage;

public:
	ErasedFunction() noexcept { storage.desc = function_helper::EmptyTypeDescriptor<R, Args...>(); }

	ErasedFunction(ErasedFunction const& other) { other.storage.desc->Copy(&storage, &other.storage); }
	ErasedFunction(ErasedFunction&& other) noexcept { other.storage.desc->Move(&storage, &other.storage); }

	template<typename T>
		requires(!IsErasedFunction<std::decay_t<T>>::value && std::invocable<std::decay_t<T>, Args...>)
	ErasedFunction(T&& val)
	{
		using DecT = std::decay_t<T>;
		if constexpr (function_helper::is_small_object_v<DecT>)
			new (&storage.buf) DecT(std::forward<T>(val));
		else
			*reinterpret_cast<const DecT**>(&storage.buf) = new DecT(std::forward<T>(val));
		storage.desc = function_helper::FunctionTraits<DecT>::template Descr<R, Args...>();
	}

	ErasedFunction& operator=(ErasedFunction const& rhs)
	{
		if (this == &rhs) return *this;
		storage.Copy(rhs.storage);
		return *this;
	}
	ErasedFunction& operator=(ErasedFunction&& rhs) noexcept
	{
		if (this == &rhs) return *this;
		storage.desc->Destroy(&storage);
		storage.desc = function_helper::EmptyTypeDescriptor<R, Args...>();
		rhs.storage.desc->Move(&storage, &rhs.storage);
		return *this;
	}

	~ErasedFunction() { storage.desc->Destroy(&storage); }

	explicit operator bool() const noexcept { return storage.desc != function_helper::EmptyTypeDescriptor<R, Args...>(); }

	R operator()(Args... args) const { return storage.desc->Invoke(&storage, std::forward<Args>(args)...); }

	GetGradTypeT<R, Args...> grad() const { return storage.desc->Grad(&storage); }

	template<typename T>
	T* target() noexcept
	{
		if (function_helper::FunctionTraits<T>::template Descr<R, Args...>() != storage.desc) return nullptr;
		if constexpr (function_helper::is_small_object_v<T>)
			return storage.template SmallCast<T>();
		else
			return storage.template BigCast<T>();
	}

	template<typename T>
	T const* target() const noexcept
	{
		if (function_helper::FunctionTraits<T>::template Descr<R, Args...>() != storage.desc) return nullptr;
		if constexpr (function_helper::is_small_object_v<T>)
			return storage.template SmallCast<T>();
		else
			return storage.template BigCast<T>();
	}
};
