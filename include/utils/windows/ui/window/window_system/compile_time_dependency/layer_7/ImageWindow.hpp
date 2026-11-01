#pragma once

#include <utils/windows/ui/window/window_system/compile_time_dependency/layer_6/Window.hpp>
#include <utils/windows/ui/gdi/dc/s_hdc.hpp>
#include <utils/windows/ui/gdi/begin_paint.hpp>
#include <utils/windows/ui/gdi/dc/draw_image.hpp>
#include <utils/image/Image2D.hpp>

namespace utils::windows::ui::window::window_system::compile_time_dependency::layer_7
{
	namespace inner::ImageWindow
	{
		//内容窗口
		class ContentWindow final : public layer_6::Window
		{
		private:
			//简便写法
			using LeafLayout = layer_4::layout::method::leaf_layout::LeafLayout;
		public:
			utils::value::property::Property<std::shared_ptr<utils::image::Image2D>> & m_image;
			utils::value::property::Property<utils::color::Color> & m_background_color;
		public:
			ContentWindow(
				utils::value::property::Property<std::shared_ptr<utils::image::Image2D>> & image,
				utils::value::property::Property<utils::color::Color> & background_color)
				: m_image{ image }
				, m_background_color{ background_color }
			{
			}
		private:
			friend class layer_2::WindowManager;

			static DWORD get_default_style()
			{
				return WS_CHILD | WS_VISIBLE;
			}
		private:
			LRESULT wnd_proc_3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
			{
				switch(uMsg)
				{
				case WM_CREATE:
					on_message_create();
					break;
				case WM_SIZE:
					on_message_size();
					break;
				case WM_PAINT:
					on_message_paint();
					break;
				case WM_ERASEBKGND:
					return 1;	//已擦除
				}
				return DefWindowProcW(hwnd, uMsg, wParam, lParam);
			}
		private:
			//响应 WM_CREATE 消息
			void on_message_create()
			{
				//当图像改变时
				m_image += [this]()
				{
					//更新布局需求
					update_layout_requirement();

					//重新绘制
					get_s_hwnd().invalidate_window();
				};

				//设置布局
				m_layout_manager.set_layout_method<LeafLayout>();
			}
			//响应 WM_SIZE 消息
			void on_message_size()
			{
				//重新绘制
				get_s_hwnd().invalidate_window();
			}
			//响应 WM_PAINT 消息
			void on_message_paint()
			{
				//窗口句柄
				const auto hwnd = get_hwnd();

				//绘制
				utils::windows::ui::gdi::begin_paint::with_memdc(hwnd, [&](HDC hdc)
				{
					//获取图像
					const auto image = m_image.get_value();

					//存在图像
					if(image)
					{
						//窗口大小
						const auto window_size = get_s_hwnd().get_client_size();

						//简单检查
						utils_assert(window_size == image->get_size());

						//绘制图像
						utils::windows::ui::gdi::dc::draw_image(hdc, *image, {});
					}
					else
					{
						//简便声明
						using s_hdc = utils::windows::ui::gdi::dc::s_hdc;

						//获取背景色
						const auto background_color = m_background_color.get_value();

						//填充背景
						s_hdc(hdc).draw_rectangle_solid(background_color);
					}
				});
			}
		private:
			//更新布局需求
			void update_layout_requirement()
			{
				//获取图像
				const auto image = m_image.get_value();

				//获取图像大小
				const auto image_size = image->get_size();

				//获取布局
				auto & leaf_layout = m_layout_manager.get_layout_method<LeafLayout>();

				//设置需求
				leaf_layout.set_requirement(image_size);
			}
		};
	}

	//图像窗口
	class ImageWindow final : public layer_6::Window
	{
	public:
		//简便写法
		using AlignValue = utils::ui::layout::AlignValue;
		using BoxLayout = layer_4::layout::method::box_layout::BoxLayout;
	public:
		//图像
		utils::value::property::Property<std::shared_ptr<utils::image::Image2D>> m_image;
		//背景颜色
		utils::value::property::Property<utils::color::Color> m_background_color{utils::color::Black};
	private:
		//内容窗口
		std::unique_ptr<inner::ImageWindow::ContentWindow> m_content_window;
	public:
		//设置图像
		void set_image(std::shared_ptr<utils::image::Image2D> image)
		{
			//设置
			m_image.set_value(image);
		}
	private:
		friend class layer_2::WindowManager;

		static DWORD get_default_style()
		{
			return WS_CHILD | WS_VISIBLE;
		}
	private:
		LRESULT wnd_proc_3(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override final
		{
			switch(uMsg)
			{
			case WM_CREATE:
				on_message_create();
				break;
			case WM_SIZE:
				on_message_size();
				break;
			case WM_PAINT:
				on_message_paint();
				break;
			case WM_ERASEBKGND:
				return 1;	//已擦除
			}
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	private:
		//响应 WM_CREATE 消息
		void on_message_create()
		{
			//当图像改变时
			m_image += [this]()
			{
				//重新绘制
				get_s_hwnd().invalidate_window();
			};
			//当背景颜色改变时
			m_background_color += [this]()
			{
				//重新绘制
				get_s_hwnd().invalidate_window();
			};

			//设置布局
			auto & layout = m_layout_manager.set_layout_method<BoxLayout>();

			//设置对齐方式
			layout.set_align({AlignValue::Middle, AlignValue::Middle});

			//创建内容窗口
			m_window_manager.create_window<inner::ImageWindow::ContentWindow>(m_image, m_background_color);
		}
		//响应 WM_SIZE 消息
		void on_message_size()
		{
			//重新绘制
			get_s_hwnd().invalidate_window();
		}
		//响应 WM_PAINT 消息
		void on_message_paint()
		{
			//窗口句柄
			const auto hwnd = get_hwnd();

			//绘制
			utils::windows::ui::gdi::begin_paint::with_memdc(hwnd, [&](HDC hdc)
			{
				//简便声明
				using s_hdc = utils::windows::ui::gdi::dc::s_hdc;

				//获取背景色
				const auto background_color = m_background_color.get_value();

				//填充背景
				s_hdc(hdc).draw_rectangle_solid(background_color);
			});
		}
	};
}