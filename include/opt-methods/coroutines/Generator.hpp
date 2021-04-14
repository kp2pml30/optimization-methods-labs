#pragma once

#include <coroutine>
#include <variant>
#include <memory>

/**
 * promise object through which coroutine submits results/exceptions base
 */
template<typename T>
struct PromiseBase
{
	std::variant<T const*, std::exception_ptr> value;

	std::suspend_always initial_suspend() noexcept { return {}; }
	std::suspend_always final_suspend() noexcept { return {}; }
	std::suspend_always yield_value(T const& other)
	{
		value = std::addressof(other);
		return {};
	}
	void return_void() {}

	template <typename Expression>
	Expression&& await_transform(Expression&& expression)
	{
		static_assert(sizeof(expression) == 0,
			"co_await is not supported in coroutines of type Generator");
		return std::forward<Expression>(expression);
	}
	void unhandled_exception() { value = std::current_exception(); }
	void rethrow_if_failed()
	{
		if (value.index() == 1)
			std::rethrow_exception(std::get<1>(value));
	}
};

/**
 * concrete promise
 */
template<typename T, typename CRTP_Child = PromiseBase<T>>
struct Promise : PromiseBase<T>
{
	auto get_return_object() { return static_cast<CRTP_Child*>(this); }
};

/**
 * base coroutine handle wrapper
 */
template <typename T, std::derived_from<PromiseBase<T>> PromiseT = Promise<T>>
struct Generator
{
	using promise_type = PromiseT;
	using handle_type = std::coroutine_handle<PromiseT>;
	handle_type handle{nullptr};

	Generator(PromiseT* promise) :
		handle(handle_type::from_promise(*promise))
	{}
	Generator() = default;
	Generator(Generator const&) = delete;
	Generator& operator=(Generator const&) = delete;
	Generator(Generator&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
	Generator& operator=(Generator&& other) noexcept
	{
		if (this != &other)
		{
			reset();
			handle = other.handle;
			other.handle = nullptr;
		}
		return *this;
	}
	~Generator()
	{
		reset();
	}

	void reset()
	{
		if (handle)
			handle.destroy();
	}

	bool next()
	{
		assert(!handle.done());
		handle.resume();
		return !handle.done();
	}

	operator bool() const noexcept
	{
		return static_cast<bool>(handle);
	}

	bool hasValue() const
	{
		return handle.promise().value.index() == 0;
	}

	T const& getValue() const
	{
		return *std::get<0>(handle.promise().value);
	}
};
