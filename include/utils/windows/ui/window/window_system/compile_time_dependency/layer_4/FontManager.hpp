#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_3/Window.hpp>
#include <utils/event/Event.hpp>
#include <utils/windows/ui/gdi/object/not_need_release.hpp>
#include <utils/value/property/VirtualProperty.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_4
{
	//向前声明
	class Window;

	class FontManager
	{
	public:
		utils::value::property::VirtualProperty<HFONT> m_font;
	private:
		utils::value::property::VirtualPropertyController<HFONT> m_font_controller{m_font};
		std::optional<HFONT> m_stored_font;
		layer_3::Window & m_window;
	private:
		friend class layer_4::Window;

		FontManager(layer_3::Window & window)
			: m_window{window}
		{
			m_font_controller.set_get_function([this]() -> HFONT
			{
				//转发
				return get_font();
			});
			m_font_controller.set_set_function([this](HFONT hont)
			{
				//已为新值
				if(m_stored_font.has_value() && hont == m_stored_font)
					return;

				//保存新值
				m_stored_font = hont;

				//向 当前窗口/子窗口/子子窗口，发送 字体已修改 的通知
				broadcast_font_changed();
			});
		}
	private:
		HFONT get_font() const
		{
			//当前窗口被直接设置过
			if(m_stored_font)
			{
				//给出
				return *m_stored_font;
			}

			//向父窗口查询
			//如果有父窗口，则转发，否则，返回默认字体

			//获取父窗口
			auto parent = m_window.m_window_manager.get_parent_window<layer_3::Window>();
			if(!parent)
			{
				//返回默认字体
				return get_default_font();
			}

			//获得父窗口的字体管理器
			auto & font_manager = parent->get_font_manager();

			//转发
			return font_manager.get_font();
		}
	private:
		static HFONT get_default_font()
		{
			return utils::windows::ui::gdi::object::not_need_release::get_font(u8"微软雅黑", 22);
		}
	private:
		void broadcast_font_changed(bool is_first = true) const
		{
			//如果不是当前窗口的字体发生了变化，当前窗口的字体已经在之前被用户设置过字体了，
			//    则不理会此次广播，也不再继续向下传播
			if(!is_first && m_stored_font)
				return;

			//向当前层次的窗口，发出事件
			m_font_controller.changed();

			//向下层子窗口，继续广播
			for(const auto & window : m_window.m_window_manager.get_all_window<layer_3::Window>())
			{
				//获得子窗口的字体管理器
				auto & font_manager = window->get_font_manager();

				font_manager.broadcast_font_changed(false);
			}
		}
	};
}