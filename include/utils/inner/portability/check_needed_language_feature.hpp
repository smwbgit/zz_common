#pragma once

#include <version>

#if __cpp_lib_three_way_comparison
#else
#	error
#endif

#ifdef __has_cpp_attribute
#  if __has_cpp_attribute(unlikely)
#  else
#    error
#  endif
#endif

#ifdef __has_cpp_attribute
#  if __has_cpp_attribute(maybe_unused)
#  else
#    error
#  endif
#endif
