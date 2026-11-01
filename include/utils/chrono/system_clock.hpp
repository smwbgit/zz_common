#pragma once

#include <chrono>

namespace utils::chrono
{
	//因为std::chrono::system_clock，在序列化时，会面临，各平台，底层精度不同的问题，而导致着，
	//    反序列化后，可能和原数据不一致的问题。所以，直接列出一个类
	struct system_clock
	{
		using rep                       = long long;
        using period                    = std::nano;
        using duration                  = std::chrono::nanoseconds;
		using time_point                = std::chrono::time_point<system_clock>;
		static constexpr bool is_steady = false;

	public:
		[[nodiscard]]
		static time_point now() noexcept
		{
			//获取系统时间
			const auto _Now = std::chrono::system_clock::now();

			//相对于1970年1月1日，经过的时间
			const auto _Now_to_1970 = _Now - std::chrono::system_clock::from_time_t(0);
			
			//相对于1970年1月1日，经过的纳秒数
			//todo 检查一下，是否会溢出
			const auto _Now_to_1970_ns =
				std::chrono::duration_cast<std::chrono::nanoseconds>(_Now_to_1970).count();

			//拼出
			const auto _Now_ = time_point(duration(_Now_to_1970_ns));

			//给出
			return _Now_;
		}

		[[nodiscard]]
		static std::time_t to_time_t(const time_point & _Time) noexcept
		{
			return std::chrono::duration_cast<std::chrono::seconds>(_Time.time_since_epoch()).count();
		}

		[[nodiscard]]
		static time_point from_time_t(std::time_t _Tm) noexcept
		{
			return time_point{std::chrono::seconds{_Tm}};
		}
	};
}