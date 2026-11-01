#pragma once

#include <vector>
#include <memory>
#include <type_traits>
#include "utils/assert.hpp"
#include "utils/copy_on_write/shared_ptr.hpp"	//todo: 看看可不可以把to_type放入类型本身的头文件里

namespace utils
{
	//向前声明
	template<typename to_T, typename from_T>
	to_T to_type(const from_T & value);

	namespace inner::to_type
	{
		template<typename to_T, typename from_T>
		struct s_conv;
#pragma region std::shared_ptr<T>
		//特化: std::shared_ptr<T>
		template<typename to_T, typename from_T>
		struct s_conv<std::shared_ptr<to_T>, std::shared_ptr<from_T>>
		{
			static std::shared_ptr<to_T> conv(const std::shared_ptr<from_T> & value)
			{
				//简单检查
				static_assert(std::is_class_v<from_T>);
				static_assert(std::is_class_v<to_T>);

				std::shared_ptr<to_T> re = std::dynamic_pointer_cast<to_T>(value);

				//简单检查
				utils_assert(re);

				return re;
			}
		};
		//特化: std::vector<std::shared_ptr<T>>
		template<typename to_T, typename from_T>
		struct s_conv<std::vector<std::shared_ptr<to_T>>, std::vector<std::shared_ptr<from_T>>>
		{
			static std::vector<std::shared_ptr<to_T>> conv(const std::vector<std::shared_ptr<from_T>> & value)
			{
				//简单检查
				static_assert(std::is_class_v<from_T>);
				static_assert(std::is_class_v<to_T>);

				std::vector<std::shared_ptr<to_T>> re;
				for (const auto p : value)
				{
					re.push_back(utils::to_type<std::shared_ptr<to_T>>(p));
				}
				return re;
			}
		};
#pragma endregion
#pragma region utils::copy_on_write::shared_ptr<T>
#if 0
		//特化: utils::copy_on_write::shared_ptr<T>
		template<typename to_T, typename from_T>
		struct s_conv<utils::copy_on_write::shared_ptr<to_T>, utils::copy_on_write::shared_ptr<from_T>>
		{
			static utils::copy_on_write::shared_ptr<to_T> conv(const utils::copy_on_write::shared_ptr<from_T> & value)
			{
				//简单检查
				static_assert(std::is_class_v<from_T>);
				static_assert(std::is_class_v<to_T>);

				utils::copy_on_write::shared_ptr<to_T> re =
					utils::copy_on_write::dynamic_pointer_cast<to_T>(value);

				//简单检查
				utils_assert(re);

				return re;
			}
		};
#endif
		//特化: std::vector<utils::copy_on_write::shared_ptr<T>>
		template<typename to_T, typename from_T>
		struct s_conv<std::vector<utils::copy_on_write::shared_ptr<to_T>>, std::vector<utils::copy_on_write::shared_ptr<from_T>>>
		{
			static std::vector<utils::copy_on_write::shared_ptr<to_T>> conv(const std::vector<utils::copy_on_write::shared_ptr<from_T>> & value)
			{
				//简单检查
				static_assert(std::is_class_v<from_T>);
				static_assert(std::is_class_v<to_T>);

				std::vector<utils::copy_on_write::shared_ptr<to_T>> re;
				for (const auto & p : value)
				{
					re.push_back(utils::copy_on_write::shared_ptr<to_T>{p});
				}
				return re;
			}
		};
#pragma endregion
	}

	//转换到指定类型
	//note: 转换失败则抛出异常
	template<typename to_T, typename from_T>
	to_T to_type(const from_T & value)
	{
		return inner::to_type::s_conv<to_T, from_T>().conv(value);
	}
}