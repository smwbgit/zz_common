#pragma once

#include <utils/image/Image2D.hpp>
#include <utils/time/steady_detection/SteadyDetection.hpp>

namespace utils::image::compare
{
	using SteadyDetection = utils::time::steady_detection::SteadyDetection<utils::image::Image2D>;
}
