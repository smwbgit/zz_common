#pragma once

//简单检查
#if defined(utils_on_windows)
#elif defined(utils_on_linux)
#elif defined(utils_on_android)
#else
#	error
#endif

#include <utils/inner/portability/parameter_description.hpp>
#include <utils/inner/portability/check_needed_language_feature.hpp>
#include <utils/inner/portability/disable_warning.hpp>