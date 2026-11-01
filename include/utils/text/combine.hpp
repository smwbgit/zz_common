#pragma once

#include <vector>
#include <string>

//合并
namespace utils::text::combine
{
	//把一些字符串，合并为一个
	class Combiner
	{
	public:
		struct s_params
		{
			struct s_optional
			{
				std::u8string separator;	//单词间的分隔符
				std::u8string prefix;		//每个单词的前缀
				std::u8string postfix;	//每个单词的后缀
			} optional;
		} m_params;
	private:
		std::u8string m_text;
		bool m_is_first{true};
	public:
		void add(const std::u8string & text)
		{
			if(m_is_first)
				m_is_first = false;
			else
				m_text += m_params.optional.separator;

			m_text += m_params.optional.prefix + text + m_params.optional.postfix;
		}
		void add_many(const std::vector<std::u8string> & texts)
		{
			for(const auto & text : texts)
				add(text);
		}
		const std::u8string & get_text() const
		{
			return m_text;
		}
	};
	//特化
	inline std::u8string combine(
		const std::vector<std::u8string> & texts,
		const std::u8string & separator,
		const std::u8string & prefix = u8"",
		const std::u8string & postfix = u8"")
	{
		//准备数据
		Combiner combiner;
		combiner.m_params.optional.separator = separator;
		combiner.m_params.optional.prefix = prefix;
		combiner.m_params.optional.postfix = postfix;

		//组合
		combiner.add_many(texts);

		//给出
		return combiner.get_text();
	}
}