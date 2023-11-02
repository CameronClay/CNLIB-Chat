#ifndef FUNCUTILS_H
#define FUNCUTILS_H

#include <functional>
#include <optional>
#include <memory>
#include <utility>
#include <cstddef>
#include <type_traits>

//Hack to allow binding of functor/lambda function/std::bind expression to a raw function pointer
namespace FuncUtils
{
	template<unsigned ID, typename Functor>
    std::optional<Functor>& get_local()
	{
        static std::optional<Functor> local{};
		return local;
	}

	template<unsigned ID, typename Functor, typename... Args>
    typename std::invoke_result_t<Functor, Args...> wrapper(Args... args) //some reason having Args&& here causes some args to be passed as pointer
	{
        return get_local<ID, Functor>().value()(std::forward<Args>(args)...); //create wrapper to call functor with a simple function
	}

	template<typename RT, typename... Args>
	struct Func
	{
        using type = RT(*)(Args...); //some reason having Args&& here causes some args to be passed as pointer
	};

	template<std::size_t ID, typename... Args>
	struct WrapperHelper
	{
		template<typename Functor>
        static typename Func<std::invoke_result_t<Functor, Args...>, Args...>::type get_wrapper(Functor&& f)
        {
            (get_local<ID, Functor>()).emplace(std::move(f));
			return wrapper<ID, Functor, Args...>;
		}
	};
}

//ex: FuncUtils::WrapperHelper<0, int>::get_wrapper(std::bind(&SomeStruct::some_method2, local[0], std::placeholders::_1))

#endif
