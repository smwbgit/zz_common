#pragma once

#include "utils/throw.hpp"

namespace utils::network
{
	//进行网络类动作时发生异常
	class Exception : public utils::Exception
	{
	};
}