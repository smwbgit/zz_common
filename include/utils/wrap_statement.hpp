#pragma once

//对一个语句进行包裹,使其能被当做一个参数传入其他函数
//example:  
//	if (utils::throwed_utils_exception(utils_wrap_statement(utils::data_encoding::url::decode(to, from))))
//	{
//		...
//	}
#define utils_wrap_statement(a) ([&](){(a);})