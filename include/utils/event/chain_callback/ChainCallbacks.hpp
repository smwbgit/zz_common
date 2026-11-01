#pragma once

#include <functional>
#include <optional>
#include <vector>

namespace utils::event::chain_callback
{
	//note: 非线程安全
	template<typename result_T, typename ... params_T>
	class ChainCallbacks
	{
	public:
		using callback_T = std::optional<result_T>(params_T...);
	private:
		std::vector<std::function<callback_T>> m_callbacks;
	public:
		std::optional<result_T> call(params_T ... params) const
		{
			for(const auto & callback : m_callbacks)
			{
				const auto result = callback(params ...);
				if(result.has_value())
					return result;
			}
			return {};
		}
		void add(std::function<callback_T> callback)
		{
			m_callbacks.push_back(callback);
		}
	};
}