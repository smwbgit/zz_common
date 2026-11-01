#pragma once

#include <functional>
#include <utils/type_traits.hpp>

namespace utils
{
	//为模板函数提供支持。当参数是std::function<..>时，使用此函数包装一下，可以使模板函数识别到模板参数
	template<typename T_lambda, typename T_function = utils::type_traits::lambda<T_lambda>::function_t>
	inline std::function<T_function> adapt_template(T_lambda && lambda)
	{
		return {std::move(lambda)};
	}
}