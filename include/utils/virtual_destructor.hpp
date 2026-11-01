#pragma once

namespace utils
{
	//提供一个虚析构函数
	//note: 可供接口类使用
	class virtual_destructor
	{
	public:
		virtual ~virtual_destructor() {};
	};
}