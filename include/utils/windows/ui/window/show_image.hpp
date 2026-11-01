#pragma once

#include <utils/text/format.hpp>
#include <utils/image/color.hpp>
#include <utils/windows/window/size.hpp>
#include <utils/windows/window/create.hpp>
#include <utils/windows/window/position.hpp>
#include <utils/windows/window/paint.hpp>
#include <utils/windows/window/dc/DIBitmap.hpp>
#include <utils/windows/window/dc/MemoryDC.hpp>

namespace utils::windows::window
{
	namespace inner
	{
		class ImageWindow
		{
		private:
			utils::image::Image2D m_image;
			std::unique_ptr<utils::windows::window::paint::dc::MemoryDC> m_memdc;

		public:
			void set_args(utils::image::Image2D && image)
			{
				m_image = std::move(image);
			}
			LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
			{
				switch (uMsg)
				{
				case WM_CREATE:
					return 0;
				case utils::windows::window::create::user_message::UM_CREATE:
					utils::windows::window::size::set_client_size(hwnd, m_image.get_size());
					if(utils::windows::window::size::is_bigger_than_workarea(m_image.get_size()))
						utils::windows::window::size::最大化(hwnd);
					else
						utils::windows::window::position::center_window(hwnd);
					return 0;
				case WM_SIZE:
				{
					InvalidateRect(hwnd, 0, FALSE);
					return 0;
				}
				case WM_PAINT:
				{
					utils::windows::window::paint::begin_paint_with_memdc(hwnd, [this, hwnd](HDC memdc)
					{
						auto client_size = utils::windows::window::size::get_client_size(hwnd);
						if (client_size.i <= 0 || client_size.j <= 0) return;

						//填充背景
						{
							auto color = utils::image::color::Black;
							HBRUSH hBrush = CreateSolidBrush(RGB(color.r, color.g, color.b));
							RECT rect = {0, 0, (LONG)client_size.i, (LONG)client_size.j};
							FillRect(memdc, &rect, hBrush);
							DeleteObject(hBrush);
						}

						if (!m_memdc)
						{
							m_memdc = std::make_unique<utils::windows::window::paint::dc::MemoryDC>(
								memdc, m_image.get_size());

							utils::windows::window::paint::dc::DIBitmap dibitmap{m_image};
							dibitmap.draw_to(m_memdc->get_hdc());
						}
						
						//能显示下
						if (client_size.j >= m_image.get_size().j)
						{
							BitBlt(memdc, 0, 0, m_image.get_size().i, m_image.get_size().j,
								m_memdc->get_hdc(), 0, 0, SRCCOPY);
						}
						//不能显示下，缩放它
						else
						{
							utils::Index2D dest_size{
								(int64_t)(1. * m_image.get_size().i / m_image.get_size().j * client_size.j),
								client_size.j
							};

							//todo 使用缩放质量更高的方式
							auto old_mode = SetStretchBltMode(memdc, STRETCH_DELETESCANS);
							utils_assert(old_mode);
							StretchBlt(memdc, 0, 0, dest_size.i, dest_size.j, m_memdc->get_hdc(), 0, 0,
								m_image.get_size().i, m_image.get_size().j, SRCCOPY);
							utils_assert(SetStretchBltMode(memdc, old_mode));
						}

					});
					return 0;
				}
				case WM_CLOSE:
					break;
				case WM_DESTROY:
					PostQuitMessage(0);		//退出当前线程
					break;
				case WM_NCDESTROY:
					break;
				}

				return DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
		};
	}
	//显示一张2D图像，在一个窗口中。不会阻塞，立即返回。窗口会在点击‘关闭按钮’后关闭
	inline void show_image(utils::image::Image2D && image, const std::u8string & title = u8"图片")
	{
		auto title_more = utils::text::format::format(u8"%s(%lld*%lld)", title.c_str(), image.get_size().i, image.get_size().j);
		utils::windows::window::create::in_new_thread<inner::ImageWindow>(
			u8"utils::windows::window::show_image",
			0,
			WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME, 0, 0, 100, 100, title_more,
			std::move(image)
		);
	}
}