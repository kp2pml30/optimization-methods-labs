#pragma once

#include <coroutine>
#include <variant>
#include <memory>

template <typename T>
struct Generator
{
	struct promise_type
	{
		std::variant<T const*, std::exception_ptr> value;
		promise_type& get_return_object() { return *this; }
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
	using handle_type = std::coroutine_handle<promise_type>;
	handle_type handle{nullptr};

	Generator(promise_type& promise) :
		handle(handle_type::from_promise(promise))
	{
	}
	Generator() = default;
	Generator(Generator const&) = delete;
	Generator &operator=(Generator const&) = delete;
	Generator(Generator&& other) : handle(other.handle) { other.handle = nullptr; }
	Generator &operator=(Generator&& other) noexcept
	{
		if (this != &other)
		{
			handle = other.handle;
			other.handle = nullptr;
		}
		return *this;
	}
	~Generator()
	{
		if (handle)
			handle.destroy();
	}

	bool next() {
		if (!handle.done())
			handle.resume(); /// TODO: properly handle different precisions on solver level
		return !handle.done();
	}

	operator bool() const {
		return static_cast<bool>(handle);
	}

	const T &get_value() {
		return *std::get<0>(handle.promise().value);
	}
};
