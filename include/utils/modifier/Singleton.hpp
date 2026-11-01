#pragma once

#include <utils/modifier/Modifier.hpp>
#include <memory>

namespace utils::modifier
{
	//单例模式
	template<typename T>
	class Singleton
	{
	public:
		static T & get_unique_object()
		{
			static std::unique_ptr<T> object = std::make_unique<T>();
			return *object;
		}
	};
}