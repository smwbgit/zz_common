#pragma once

#include "utils/assert.hpp"

namespace utils::statistics
{
	//传入一些值后，给出其中的最大最小值
	template<typename T>
	class maxmin_finder
	{
	public:
		void meet_one_value(const T & value)
		{
			if (m_is_meeted_value == false)
			{
				m_is_meeted_value = true;
				m_max = value;
				m_min = value;
				return;
			}
			if (value > m_max)
				m_max = value;
			if (value < m_min)
				m_min = value;
		}
		//获得最大值
		T get_max() const
		{
			utils_assert(m_is_meeted_value);
			return m_max;
		}
		//获得最小值
		T get_min() const
		{
			utils_assert(m_is_meeted_value);
			return m_min;
		}
	private:
		bool m_is_meeted_value{};
		T m_max{};
		T m_min{};
	};
}

//单词频率
namespace utils::statistics::word_frequency
{
	//输入一些单词(有一些是重复的), 统计每个单词的出现次数和出现频率
	class Box
	{
	public:
		//统计结果
		class Report
		{
		public:
			class Entry
			{
			public:
				std::string word;	//单词
				uint64_t count;		//出现次数
				float frequency;	//出现频率, 0到1
			};
			std::vector<Entry> words;
			uint64_t total;		//统计总数, i.e. 每个单词出现次数之和
		};

	private:
		std::map<std::string, uint64_t> m_count;

	public:
		//添加一个单词
		void add(_In_ const std::string & word)
		{
			m_count[word]++;
		}
		//生成统计结果
		void get_report(_Out_ Report & report)
		{
			Report _report{};

			//统计总数
			for (const auto & one : m_count)
				_report.total += one.second;

			//填充
			for (const auto & one : m_count)
				_report.words.push_back({one.first, one.second, 1.f * one.second / _report.total});
			
			//排序
			std::sort(_report.words.begin(), _report.words.end(), [](const auto & a, const auto & b)
			{
				return a.count > b.count;
			});

			//给出
			report = std::move(_report);
		}
	};
}