#pragma once

#include <string>
#include <vector>
#include <winuser.h>
#include "utils/assert.hpp"
#include "utils/computer_basic.hpp"
#include "utils/BufferWriter.hpp"
#include "utils/windows/api/dpi.hpp"

namespace utils::windows::mfc
{
	//生成一个供CDialog::InitModalIndirect()使用的对话框模板
	class DialogTemplate
	{
	public:
		//windows预定义的控件
		enum class Control
		{
			_static,
			edit,
			button,
			combo_box,
		};
		
		//设置对话框基本信息
		void set(_In_ const DLGTEMPLATE & dlg_template, _In_ const std::string & title)
		{
			m_other._template = dlg_template;
			m_other.title = title;

			//修正矩形: 这里传入的xy和宽高似乎被放大了2次后显现出来的, 所以在这里先除一次
			{
				utils::windows::api::dpi::get_original_size(m_other._template.x);
				utils::windows::api::dpi::get_original_size(m_other._template.y);
				utils::windows::api::dpi::get_original_size(m_other._template.cx);
				utils::windows::api::dpi::get_original_size(m_other._template.cy);
			}
		}

		//向对话框添加一个控件
		void add_one_item(_In_ const DLGITEMTEMPLATE & item_template, _In_ const Control ctrl, _In_ const std::string & name)
		{

			s_item item;
			{
				item._template = item_template;
				item.ctrl = ctrl;
				item.name = name;
			}

			//修正矩形: 这里传入的xy和宽高似乎被放大了2次后显现出来的, 所以在这里先除一次
			{
				utils::windows::api::dpi::get_original_size(item._template.x);
				utils::windows::api::dpi::get_original_size(item._template.y);
				utils::windows::api::dpi::get_original_size(item._template.cx);
				utils::windows::api::dpi::get_original_size(item._template.cy);
			}

			m_items.push_back(std::move(item));
		}
		//使用模板
		void use_to(_In_ CDialog* dialog, _In_ CWnd* parent = 0)
		{
			//生成模板
			get_data(m_data);

			//指定模板
			dialog->InitModalIndirect((LPCDLGTEMPLATE)m_data.data(), parent);
		}
	private:
		WORD get_atom_value(_In_ const Control control)
		{
			switch(control)
			{
			case Control::_static:		return 0x0082;
			case Control::edit:			return 0x0081;
			case Control::button:		return 0x0080;
			case Control::combo_box:	return 0x0085;
			}
			utils_throw();
		}
		void get_data(_Out_ std::vector<uint8_t> & data)
		{
			//参见: https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-dlgtemplate?redirectedfrom=MSDN
			//参见: https://docs.microsoft.com/zh-cn/windows/win32/api/winuser/ns-winuser-dlgitemtemplate?redirectedfrom=MSDN

			//简单检查
			utils_assert((m_other._template.style & DS_SETFONT) == 0);

			//修正数据
			utils::computer_basic_function::store_this_integer(m_other._template.cdit, m_items.size());

			//写入缓冲区，然后读取它
			{
				utils::BufferWriter output;

				//写入对话框
				output.write(m_other._template);
				output.write<WORD>(0); // no menu
				output.write<WORD>(0); // use default window class
				{
					std::wstring tmp;
					utils::character_encoding::convert_cp936_to_unicode(tmp, m_other.title);
					output.write(tmp.c_str(), (tmp.length() + 1) * 2);
				}

				//写入所有控件
				for (const auto & item : m_items)
				{
					output.align(4);	//对齐到DWORD边界
					output.write(item._template);
					output.write<WORD>(0xFFFF);  // indicating atom value
					output.write<WORD>(get_atom_value(item.ctrl));   // atom value for the control
					{
						std::wstring tmp;
						utils::character_encoding::convert_cp936_to_unicode(tmp, item.name);
						output.write(tmp.c_str(), (tmp.length() + 1) * 2);
					}
					output.write<WORD>(0);	//No custom data
				}

				//读取并传出
				output.get_buffer_data(data);
			}
		}
		struct s_item
		{
			DLGITEMTEMPLATE _template{};
			Control ctrl{};
			std::string name;
		};
		std::vector<s_item> m_items;

		struct s_other
		{
			DLGTEMPLATE _template{};
			std::string title;
		};
		s_other m_other;

		std::vector<uint8_t> m_data;	//生成的模板数据。需要在对话框存在时也存在
	};
}