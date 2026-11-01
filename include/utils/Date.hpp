#pragma once

#include <chrono>
#include <utils/compiler/windows_header/maxmin__eliminate.inc>
#include <boost/container_hash/hash.hpp>
#include <utils/compiler/windows_header/maxmin__restore.inc>
#include <utils/set.hpp>
#include <utils/string.hpp>
#include <utils/computer_basic.hpp>
#include <utils/assert.hpp>

namespace utils
{
	//日期类   保存年月日
	class Date
	{
	public:
		uint64_t year{};
		uint64_t month{};
		uint64_t day{};

#pragma region 构造函数
		Date()
		{
		}
		Date(uint64_t year, uint64_t month, uint64_t day)
			: year{year}
			, month{month}
			, day{day}
		{
		}
		template<typename T1, typename T2, typename T3>
		Date(T1 _year, T2 _month, T3 _day)
		{
			set(_year, _month, _day);
		}
#pragma endregion

#pragma region 修改函数
		template<typename T1, typename T2, typename T3>
		void set(T1 _year, T2 _month, T3 _day)
		{
			utils::computer_basic_function::store_this_integer(year, _year);
			utils::computer_basic_function::store_this_integer(month, _month);
			utils::computer_basic_function::store_this_integer(day, _day);
		}
		//从一个形式为1911-01-01的字符串里提取日期
		void from_string(const std::string & text)
		{
			if (3 != sscanf(text.c_str(), "%llu-%llu-%llu", &year, &month ,&day))
				utils_throw();
		}
		//清零
		void clear()
		{
			year = 0;
			month = 0;
			day = 0;
		}
		//变为经过指定天数后的日期
		//days: 天数
		void pass_days(uint64_t days)
		{
			tm tt{};
			tt.tm_year = (int)(year - 1900);
			tt.tm_mon = (int)(month - 1);
			tt.tm_mday = (int)day;

			time_t ttt = mktime(&tt);
			auto t = std::chrono::system_clock::from_time_t(ttt);

			t += std::chrono::hours{days * 24};

			time_t _ttt = std::chrono::system_clock::to_time_t(t);
			auto tmp = localtime(&_ttt);
			if (nullptr == tmp)
				utils_throw();
			tm _tt = *tmp;

			year = _tt.tm_year + 1900;
			month = _tt.tm_mon + 1;
			day = _tt.tm_mday;
		}
#pragma endregion

#pragma region 查询函数
		//是否被清零
		bool is_cleared() const
		{
			return year == 0 && month == 0 && day == 0;
		}
		//是否相同
		bool operator == (const Date & o) const
		{
			if (year != o.year)
				return false;
			
			if (month != o.month)
				return false;
			
			if (day != o.day)
				return false;

			return true;
		}
		//是否不同
		bool operator != (const Date & o) const
		{
			return !(*this == o);
		}
		//此日期是否在另一个日期之前
		bool operator < (const Date & o) const
		{
			if (year < o.year)
				return true;
			else if (year > o.year)
				return false;
			
			if (month < o.month)
				return true;
			else if (month > o.month)
				return false;
			
			if (day < o.day)
				return true;
			else if (day > o.day)
				return false;

			return false;	//相同时
		}
		//此日期是否在另一个日期之后
		bool operator > (const Date & o) const
		{
			return o < *this;
		}
		//此日期是否在另一个日期之后
		bool is_after(const Date & o) const
		{
			return o < * this;
		}

		//返回形如1971-02-06的字符串
		void to_string(std::string & date_string) const
		{
			//简单检查
			utils_assert(year >= 1000 && year <= 9999);
			utils_assert(month >= 1 && month <= 12);
			utils_assert(day >= 1 && day <= 31);

			utils::string_function::format(date_string, "%d-%02d-%02d", year, month, day);
		}
#pragma endregion
	};

	namespace date_function
	{
#pragma region 查询函数
		//按年获得日期：每年一个最后日期
		//dates: 输入
		//some_dates: 输出
		inline void get_dates_by_year(_Out_ std::set<Date> & some_dates, _In_ const std::set<Date> & dates)
		{
			utils::set<uint64_t> years;
			some_dates.clear();
			for (auto iter = dates.crbegin(); iter != dates.crend(); iter++)	//为了获得最后的日期，逆向遍历
			{
				const auto & date = *iter;
				if (years.exist(date.year))
					continue;

				some_dates.insert(date);
				years.insert(date.year);
			}
		}
			
		//按月获得日期：每月一个最后日期
		//dates: 输入
		//some_dates: 输出
		inline void get_dates_by_month(_Out_ std::set<Date> & some_dates, _In_ const std::set<Date> & dates)
		{
			utils::set<std::tuple<uint64_t, uint64_t>> year_and_month;
			some_dates.clear();
			for (auto iter = dates.crbegin(); iter != dates.crend(); iter++)	//为了获得最后的日期，逆向遍历
			{
				const auto & date = *iter;
				if (year_and_month.exist(std::make_tuple(date.year, date.month)))
					continue;

				some_dates.insert(date);
				year_and_month.insert(std::make_tuple(date.year, date.month));
			}
		}

		//获得最后一个日期
		inline void get_last_date(_Out_ Date & last_date, _In_ const std::set<Date> & dates)
		{
			utils_assert(false == dates.empty());

			last_date = *dates.crbegin();	//由于set自动进行了排序，所以最后一个就是想要的日期
		}

		//获得第一个日期
		inline void get_first_date(_Out_ Date & first_date, _In_ const std::set<Date> & dates)
		{
			utils_assert(false == dates.empty());

			first_date = *dates.cbegin();	//由于set自动进行了排序，所以第一个就是想要的日期
		}
#pragma endregion
	};
}

namespace std
{
	//补充std::hash，对于类型utils::Date
	template<>
	class hash<utils::Date>
	{
	public:
		std::size_t operator()(const utils::Date & d) const
		{
			std::size_t seed = 0;
			boost::hash_combine(seed, d.year);
			boost::hash_combine(seed, d.month);
			boost::hash_combine(seed, d.day);
			return seed;
		}
	};
}