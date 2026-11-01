#pragma once

#include <utils/time/StartPosition.hpp>
#include <utils/map.hpp>

//时间线
namespace utils::time::time_line
{
	//有名字的
	class WithName
	{
	private:
		struct s_point
		{
			std::u8string name;
			std::chrono::milliseconds position;
			bool is_closed{};
		};
	private:
		//时间线的起点
		utils::time::StartPosition m_start_position;
		//全部事件点
		std::map<std::u8string, s_point> m_all_point;
	public:
		//添加事件点
		void add(const std::u8string & name, std::chrono::milliseconds position)
		{
			//保存
			utils::map(m_all_point).insert_not_exist(name, {name, position});
		}
		//是否到达了指定事件点
		//note: 每个事件点，只会进入一次
		bool is_in(const std::u8string & name)
		{
			//获取相应的事件点
			auto & point = utils::map(m_all_point).get_existed_value(name);

			//已经访问过
			if(point.is_closed)
				return false;

			//还未到达
			if(!m_start_position.is_enough_passed(point.position))
				return false;

			//更新状态
			point.is_closed = true;

			//给出
			return true;
		}
	};
}