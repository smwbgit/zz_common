#pragma once

#if defined(utils_on_windows)
#	include <sal.h>
#elif defined(utils_on_linux) || defined(utils_on_android)
#	define _In_
#	define _Out_
#	define _Inout_
#endif
