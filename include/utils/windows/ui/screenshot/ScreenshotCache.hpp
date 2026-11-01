#pragma once

#include <utils/Cache.hpp>
#include <utils/windows/ui/screenshot.hpp>

namespace utils::windows::ui::screenshot
{
	class ScreenshotCache
	{
	private:
		utils::Cache<utils::image::Image2D> m_cache;
	public:
		ScreenshotCache()
		{
			m_cache.m_get_update_value = []()
			{
				return utils::windows::ui::screenshot::get_image();
			};
		}
		const utils::image::Image2D & get_value()
		{
			//转发
			return m_cache.get_value();
		}
		void update_value()
		{
			//转发
			return m_cache.update_value();
		}
	};
}