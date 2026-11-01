#pragma once

#include <string>
#include <vector>

namespace utils::binary::data::text::评估
{
	//对一个字符串进行评估.
	//note: 返回值越大, 它有意义的可能性越大.
	//      反之, 返回值越小, 它是无意义的乱码的可能性越大
	//算法: 对每个位置进行检查, 当是一个单词时, 它的每个字母位置都变为其长度的平方(越长的单词越难匹配, 所以它的评分更高).
	//      最后,对每个字母位置求和, 再除以单词的长度(越长的text, 越容易在其中找到匹配的单词, 所以长的text, 评分更低).
	inline double 评估(const std::u8string & text)
	{
		const auto & s = text;
		if (s.empty())	return 0.;

		std::vector<uint64_t> 分数;
		分数.resize(s.length());

		auto is_word = [&](int i, int j)
		{
			std::u8string u8 = (const std::u8string&)s.substr(i, j);
			for(auto & c : u8)
			{
				if(isalpha(c))
					c = tolower(c);
			}
			return utils::text::常用词::english(u8);
		};
		auto 刷新分数 = [&](int i, int j)
		{
			//[i,i+j) 变为 max(原值,j)

			for(int k = 0; k < j; k++)
			{
				if (分数[i+k] < j)
					分数[i+k] = j;
			}
		};
		for(int i = 0; i < s.length(); i++)
		{
			for(int j = 10; j >= 1;j--)
			{
				if(i+j>s.length()) continue;

				if(is_word(i, j))
				{
					刷新分数(i,j);
					break;
				}
			}
		}
		double 评估{};
		for(int i = 0; i < s.length(); i++)
		{
			评估 += 分数[i];
		}
		评估/=s.length();
		return 评估;
	}
}
