#pragma once

#include <chrono>
#include <functional>

namespace utils::ui::concepts
{
	template<typename T>
	concept TimerManager = requires(
		T t,
		std::chrono::milliseconds time_length,
		std::function<void(bool & is_kill_timer)> meet_expired_1,
		std::function<void()> meet_expired_2)
	{
		{ t.add_timer(time_length, meet_expired_1) } -> std::same_as<void>;
		{ t.add_timer(time_length, meet_expired_2) } -> std::same_as<void>;
	};
}