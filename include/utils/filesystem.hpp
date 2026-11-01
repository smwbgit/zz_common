#pragma once

#include <filesystem>
#include <utils/throw.hpp>

namespace utils::filesystem
{
	//捕捉std::filesystem异常，转为utils_throw()异常
	template <typename T>
	inline auto care_for_std_exception(T && func) -> std::invoke_result_t<T>
	{
		//简单检查
		static_assert(std::is_invocable_v<T>);

		try
		{
			return func();
		}
		catch (std::filesystem::filesystem_error & e)
		{
			[[maybe_unused]] std::string text_for_debug = e.what();

			//printf("filesystem error: %s\n", e.what());
			text_for_debug = text_for_debug;
			utils_throw();
		}
	}
#pragma region 简单包装
	inline auto is_directory(const std::filesystem::path & path)
	{
		return care_for_std_exception( [&](){
			return std::filesystem::is_directory(path); });
	}
	inline auto file_size(const std::filesystem::path & path)
	{
		return care_for_std_exception([&]() {
			return std::filesystem::file_size(path); });
	}
#pragma endregion
}