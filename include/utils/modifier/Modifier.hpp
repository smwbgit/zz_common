#pragma once

namespace utils::modifier
{
	//修饰词
	//note: 在多重继承中，可以同时存在多个不同的修饰词
	class Modifier
	{
	public:
		virtual ~Modifier() = 0;
	};
	inline Modifier::~Modifier() { }
}