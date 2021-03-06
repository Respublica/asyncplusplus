// Copyright (c) 2013 Amanieu d'Antras
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ASYNCXX_H_
# error "Do not include this header directly, include <async++.h> instead."
#endif

namespace async {
namespace detail {

// Pseudo-void type: it takes up no space but can be moved and copied
struct fake_void {};
template<typename T> using void_to_fake_void = std::conditional<std::is_void<T>::value, fake_void, T>;
template<typename T> T&& fake_void_to_void(T&& x)
{
	return std::forward<T>(x);
}
inline void fake_void_to_void(fake_void) {}

// Check if type is a task type, used to detect task unwraping
template<typename T> struct is_task: public std::false_type {};
template<typename T> struct is_task<task<T>>: public std::true_type {};
template<typename T> struct is_task<const task<T>>: public std::true_type {};
template<typename T> struct is_task<shared_task<T>>: public std::true_type {};
template<typename T> struct is_task<const shared_task<T>>: public std::true_type {};

// Extract the result type of a task if T is a task, otherwise just return T
template<typename T> struct remove_task {
	typedef T type;
};
template<typename T> struct remove_task<task<T>> {
	typedef T type;
};
template<typename T> struct remove_task<const task<T>> {
	typedef T type;
};
template<typename T> struct remove_task<shared_task<T>> {
	typedef T type;
};
template<typename T> struct remove_task<const shared_task<T>> {
	typedef T type;
};

// Check if a type is callable with the given arguments
template<typename Func, typename... Args, typename = decltype(std::declval<Func>()(std::declval<Args>()...))> std::true_type is_callable_helper(int);
template<typename Func, typename... Args> std::false_type is_callable_helper(...);
template<typename T> struct is_callable;
template<typename Func, typename... Args> struct is_callable<Func(Args...)>: public decltype(is_callable_helper<Func, Args...>(0)) {};

// Wrapper to run a function object with an optional parameter:
// - void returns are turned into fake_void
// - fake_void parameter will invoke the function with no arguments
template<typename Func, typename = typename std::enable_if<!std::is_void<decltype(std::declval<Func>()())>::value>::type>
decltype(std::declval<Func>()()) invoke_fake_void(Func&& f)
{
	return std::forward<Func>(f)();
}
template<typename Func, typename = typename std::enable_if<std::is_void<decltype(std::declval<Func>()())>::value>::type>
fake_void invoke_fake_void(Func&& f)
{
	std::forward<Func>(f)();
	return fake_void();
}
template<typename Func, typename Param> typename void_to_fake_void<decltype(std::declval<Func>()(std::declval<Param>()))>::type invoke_fake_void(Func&& f, Param&& p)
{
	return invoke_fake_void([&f, &p] {return std::forward<Func>(f)(std::forward<Param>(p));});
}
template<typename Func> typename void_to_fake_void<decltype(std::declval<Func>()())>::type invoke_fake_void(Func&& f, fake_void)
{
	return invoke_fake_void(std::forward<Func>(f));
}

// Various properties of a continuation function
template<typename Func, typename Parent, typename = decltype(std::declval<Func>()(std::declval<Parent>().get()))>
std::true_type is_value_cont_helper(Func&&, const Parent&, int, int);
template<typename Func, typename = decltype(std::declval<Func>()())>
std::true_type is_value_cont_helper(Func&&, const task<void>&, int, int);
template<typename Func, typename = decltype(std::declval<Func>()())>
std::true_type is_value_cont_helper(Func&&, const shared_task<void>&, int, int);
template<typename Func, typename Parent, typename = decltype(std::declval<Func>()(std::declval<Parent>()))>
std::false_type is_value_cont_helper(Func&&, const Parent&, int, ...);
template<typename Func, typename Parent>
void is_value_cont_helper(Func&&, const Parent&, ...);
template<typename Parent, typename Func>
struct continuation_traits {
	typedef decltype(is_value_cont_helper(std::declval<Func>(), std::declval<Parent>(), 0, 0)) is_value_cont;
	static_assert(!std::is_void<is_value_cont>::value, "Parameter type for continuation function is invalid for parent task type");
	typedef typename std::conditional<is_value_cont::value, typename void_to_fake_void<decltype(std::declval<Parent>().get())>::type, Parent>::type param_type;
	typedef decltype(fake_void_to_void(invoke_fake_void(std::declval<Func>(), std::declval<param_type>()))) result_type;
	typedef task<typename remove_task<result_type>::type> task_type;
};

} // namespace detail
} // namespace async
