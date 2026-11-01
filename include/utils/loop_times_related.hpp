#pragma once

//和循环次数相关的操作
namespace utils::loop_times_related
{
	//忽略第一次调用
	/*
	example:
		code:
			utils::loop_times_related::ignore_first spliter;
			utils::string re;
			re += "[";
			for (auto name : names)
			{
				re += spliter.echo(", ");
				re += name.c_str();
			}
			re += "]";
		result:
			when:
				names = {"a", "b", "c"};
			then:
				re == "[a, b, c]";
	*/
	class IngoreFirst
	{
	private:
		bool m_is_first{true};

	public:
		//直接给出参数的值,除了第一次.第一次时给出空值
		template<typename T>
		T echo(const T & value)
		{
			if (m_is_first)
			{
				m_is_first = false;
				return T{};
			}
			else
			{
				return value;	//todo 看看这里怎么能不拷贝
			}
		}
		const char* echo(const char* value)
		{
			if (m_is_first)
			{
				m_is_first = false;
				return "";
			}
			else
			{
				return value;
			}
		}
	};
};