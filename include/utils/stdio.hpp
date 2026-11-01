#pragma once

#include <vector>
#include <stdio.h>

//for: 标准输入输出
namespace utils::stdio
{
	//从stdin里读入所有数据
	void read_all(_Out_ std::vector<uint8_t> & data)
	{
		data.clear();
		while (1)
		{
			uint8_t c{};
			if (1 != scanf("%c", &c))
				break;

			data.push_back(c);
		}
	}
	//向stdout写入数据
	void write(_In_ const std::vector<uint8_t> & data)
	{
		for (const auto c : data)
		{
			printf("%c", c);
		}
	}
}