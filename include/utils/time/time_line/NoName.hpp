#pragma once

#include <utils/time/StartPosition.hpp>
#include <utils/vector.hpp>

//时间线
namespace utils::time::time_line
{
	//没名字的
	class NoName
	{
	private:
		struct s_point
		{
			std::chrono::milliseconds position;
			bool is_closed{};
		};
	private:
		//时间线的起点
		utils::time::StartPosition m_start_position;
		//全部事件点
		std::vector<s_point> m_all_point;
	public:
		NoName() = default;
		template<typename T>
			requires std::convertible_to<T, std::chrono::milliseconds>
		NoName(const std::vector<T> & positions)
		{
			for(const auto & position : positions)
			{
				add(position);
			}
		}
	public:
		//添加一个事件点
		void add(const std::chrono::milliseconds position)
		{
			//简单检查
			utils_assert(m_all_point.empty() || utils::vector(m_all_point).last_one().position < position);

			//保存
			m_all_point.push_back({position});
		}
		//是否到达了下一个事件点
		bool is_in_next()
		{
			//依次检查事件点
			for(auto & point : m_all_point)
			{
				//已关闭
				if(point.is_closed)
				{
					//检查下一个
					continue;
				}

				//已到达
				if(m_start_position.is_enough_passed(point.position))
				{
					//更新状态
					point.is_closed = true;

					//给出
					return true;
				}
			}

			//给出
			return false;
		}
	};
}