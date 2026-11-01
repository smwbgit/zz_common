#pragma once

#include "utils/map.hpp"
#include "utils/IdGiver.hpp"
#include "utils/windows/mfc/BEGIN_MESSAGE_MAP__IN_CLASS.hpp"
#include "utils/windows/mfc/DialogTemplate.hpp"
#include "utils/windows/mfc/warehouse.hpp"

#if 0
namespace utils::windows::mfc::control::simple_swap
{
	//能设置tab是转移焦点还是当为字符
	class CEdit : public ::CEdit
	{
	public:
		using ::CEdit::CEdit;
#pragma region tab键
	private:
		bool m_is_want_tab{};
	public:
		void set_is_want_tab(bool is_want_tab)
		{
			m_is_want_tab = is_want_tab;
		}
	private:
		UINT OnGetDlgCode()
		{
			UINT value = __super::OnGetDlgCode();
			if (m_is_want_tab)
				value |= DLGC_WANTTAB;
			return value;
		}
#pragma endregion
	private:
		BEGIN_MESSAGE_MAP__IN_CLASS(CEdit)
			ON_WM_GETDLGCODE()
		END_MESSAGE_MAP__IN_CLASS()
	};
}
#endif

namespace utils::windows::mfc
{
	namespace inner
	{
		class Dialog_MultiLinesEditor : public CDialog
		{
#pragma region ui界面布局常量	//todo 改为能改变窗口大小
		private:
#define _2022_3_13_fix(a) ((int)((a)/1.5625*1.5*1.5 + 0.5))
			static const short m_ui_width_window = (int)(300*1.5 + 0.5);
			static const short m_ui_width_button = _2022_3_13_fix(48);
			static const short m_ui_width_before_edit = 1;
			static const short m_ui_width_after_edit = 0;
			static const short m_ui_width_edit = m_ui_width_window -  m_ui_width_before_edit - m_ui_width_after_edit;
			static const short m_ui_height_interval_above_edit = 1;	//在edit上边额外的空白
			static const short m_ui_height_edit = (int)(300*1.5 + 0.5);
			static const short m_ui_width_before_ok_button = (int)(174*1.5 + 0.5);	//在ok按钮前边额外的空白
			static const short m_ui_width_before_cancel_button = (int)(14*1.5 + 0.5);	//在cancel按钮前边额外的空白
			static const short m_ui_height_button = _2022_3_13_fix(17);
			static const short m_ui_height_interval_above_ok_button = (int)(5*1.5 + 0.5);	//在ok按钮上边额外的空白
			static const short m_ui_height_interval_below_ok_button = (int)(4*1.5 + 0.5);	//在ok按钮下边额外的空白
#pragma endregion
#pragma region 父窗口
		private:
			CWnd* m_parent{};
		public:
			void set_parent(CWnd* parent)
			{
				m_parent = parent;
			}
#pragma endregion
#pragma region 控件类型
		private:
			using ui_edit_t = CEdit;
//			using ui_edit_t = utils::windows::mfc::control::simple_swap::CEdit;
#pragma endregion
#pragma region 控件id
		private:
			static const WORD m_ui_id_button_ok = 5000;
			static const WORD m_ui_id_button_cancel = 5001;
			static const WORD m_ui_id_edit = 5002;
#pragma endregion
#pragma region 构建对话框
		private:
			bool m_is_template_setted{};
			utils::windows::mfc::DialogTemplate m_template;
		public:
			INT_PTR DoModal() override
			{
				//指定模板
				set_template();

				return __super::DoModal();
			}
		private:
			//获得控件指针
			template<typename ctrl_T>
			ctrl_T* get_ctrl(_In_ const WORD id)
			{
				auto ctrl = (ctrl_T*)GetDlgItem(id);
				utils_assert(ctrl);
				return ctrl;
			}
			void set_template()
			{
				if (m_is_template_setted)
					return;

				//创建模板
				{
					//初始化
					short y{};

					//写入一个多行Edit控件
					y += m_ui_height_interval_above_edit;
					using Control = utils::windows::mfc::DialogTemplate::Control;
					m_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOHSCROLL | ES_WANTRETURN | WS_VSCROLL | WS_HSCROLL, 0,  m_ui_width_before_edit, y, m_ui_width_edit, m_ui_height_edit, m_ui_id_edit}, Control::edit, "");
					y += m_ui_height_edit;

					//写入两个按钮
					y += m_ui_height_interval_above_ok_button;
					m_template.add_one_item(DLGITEMTEMPLATE{BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE, 0,  m_ui_width_before_ok_button, y, m_ui_width_button, m_ui_height_button, m_ui_id_button_ok}, Control::button, "确定");
					m_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD | WS_VISIBLE, 0,  m_ui_width_before_ok_button + m_ui_width_button + m_ui_width_before_cancel_button, y, m_ui_width_button, m_ui_height_button, m_ui_id_button_cancel}, Control::button, "取消");
					y += m_ui_height_button;
					y += m_ui_height_interval_below_ok_button;

					//写入对话框的基本信息
					m_template.set(DLGTEMPLATE{WS_POPUP | WS_CAPTION | WS_VISIBLE | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX, 0, 0, 0, 0, m_ui_width_window, y}, "文本编辑");
				}

				//使用模板
				m_template.use_to(this, m_parent);

				//更新状态
				m_is_template_setted = true;
			}
			BOOL OnInitDialog() override
			{
				__super::OnInitDialog();

				//把\n换为\r\n
				std::string _text;
				{
					//简单检查
					utils_assert(m_text.find('\r') == std::string::npos);

					_text = m_text;
					utils::string_function::replace_all(_text, "\n", "\r\n");
				}
				
				//初始化控件里的值
				get_ctrl<ui_edit_t>(m_ui_id_edit)->SetWindowTextA(_text.c_str());

				//设置字体
				using namespace utils::windows::mfc::warehouse::import;
				warehouse::font::SetFont(*get_ctrl<ui_edit_t>(m_ui_id_edit), warehouse::font::Id::Edit);
				warehouse::font::SetFont(*get_ctrl<CButton>(m_ui_id_button_ok), warehouse::font::Id::Button);
				warehouse::font::SetFont(*get_ctrl<CButton>(m_ui_id_button_cancel), warehouse::font::Id::Button);

				//取消选中
#if 0
		//		get_ctrl<ui_edit_t>(m_ui_id_edit)->SetSel(-1, 0, FALSE);
				{
					int a{}, b{};
					get_ctrl<ui_edit_t>(m_ui_id_edit)->GetSel(a, b);
					a = a;
				}
#endif

				//设置tab的宽度
				get_ctrl<ui_edit_t>(m_ui_id_edit)->SetTabStops(16);		//4个字符的宽度

				//设置能接受tab字符
//				get_ctrl<ui_edit_t>(m_ui_id_edit)->set_is_want_tab(true);

				return FALSE;  // return TRUE unless you set the focus to a control
								// 异常: OCX 属性页应返回 FALSE
			}
#pragma endregion
#pragma region 值
		private:
			std::string m_text;
		public:
			void set_text(_In_ const std::string & text)
			{
				//简单检查
				utils_assert(text.find('\r') == std::string::npos);

				//保存
				m_text = text;
			}
			void get_text(_Out_ std::string & text)
			{
				text = m_text;
			}
#pragma endregion
#pragma region 检查函数
		private:
			using check_func_t = std::function<bool(std::string)>;
			check_func_t m_check_func;
		public:
			void set_check_func(_In_ check_func_t check_func)
			{
				//保存
				m_check_func = check_func;
			}
#pragma endregion
#pragma region 移动控件
			//缩放后, 移动控件
			void move_window(CWnd* ctrl, int x, int y, int w, int h)
			{
				//缩放
				utils::windows::api::dpi::get_scaled_size(x);
				utils::windows::api::dpi::get_scaled_size(y);
				utils::windows::api::dpi::get_scaled_size(w);
				utils::windows::api::dpi::get_scaled_size(h);

				ctrl->MoveWindow(x, y, w, h);
			}
#pragma endregion
#pragma region 消息处理
		private:
			void on_button_ok__clicked()
			{
				//获得字符串
				utils::string text;
				{
					//获得字符串
					CString _text;
					get_ctrl<ui_edit_t>(m_ui_id_edit)->GetWindowTextA(_text);
					text = _text.GetString();

					//把\r\n转为\n
					text.replace_all("\r\n", "\n");

					//简单检查
					utils_assert(text.find('\r') == utils::string::npos);
				}

				//用户定义的检查
				if (m_check_func)
				{
					if (false == m_check_func(text))
						return;
				}

				//保存
				m_text = text;

				//关闭对话框
//				EndDialog(IDOK);
				OnOK();
			}
			void on_button_cancel__clicked()
			{
				__super::OnCancel();
//				EndDialog(IDCANCEL);
			}
			void OnCancel() override
			{
				on_button_cancel__clicked();
			}
			void OnSize(UINT nType, int cx, int cy)
			{
				__super::OnSize(nType, cx, cy);

				//放置各个控件
				if (GetDlgItem(m_ui_id_edit))
				{
					int client_w{};
					int client_h{};
					{
						CRect client_rect{};
						GetClientRect(&client_rect);

						client_w = client_rect.Width();
						client_h = client_rect.Height();
					}

					//buttons: ok, cancel
					int height_bottom_used{};
					{
						//准备数据
						int x{};
						int y{};
						int w{};
						int h{};
						int step{};
						{
							//放大
							utils::windows::api::dpi::get_scaled_size<int, false>(x, m_ui_width_window - m_ui_width_before_ok_button);
							x = client_w - x;
							utils::windows::api::dpi::get_scaled_size<int, false>(y, m_ui_height_button + m_ui_height_interval_below_ok_button);
							y = client_h - y;
							utils::windows::api::dpi::get_scaled_size<int>(w, m_ui_width_button);
							utils::windows::api::dpi::get_scaled_size<int>(h, m_ui_height_button);
							utils::windows::api::dpi::get_scaled_size<int>(step, m_ui_width_button + m_ui_width_before_cancel_button);
						}

						//放置按钮
						get_ctrl<CButton>(m_ui_id_button_ok)->MoveWindow(x, y, w, h);
						get_ctrl<CButton>(m_ui_id_button_cancel)->MoveWindow(x + step, y, w, h);

						//计算剩余面积
						utils::windows::api::dpi::get_scaled_size<int, false>(height_bottom_used, m_ui_height_interval_above_ok_button + m_ui_height_button + m_ui_height_interval_below_ok_button);
					}

					//edit
					{
						auto _before_edit = m_ui_width_before_edit;
						auto _after_edit = m_ui_width_after_edit;
						auto _above_edit = m_ui_height_interval_above_edit;

						//放大
						utils::windows::api::dpi::get_scaled_size(_before_edit);
						utils::windows::api::dpi::get_scaled_size(_after_edit);
						utils::windows::api::dpi::get_scaled_size(_above_edit);

						CRect rect{};
						rect.left = _before_edit;
						rect.top = _above_edit;
						rect.right = client_w - _after_edit;
						rect.bottom = client_h - height_bottom_used;

						get_ctrl<ui_edit_t>(m_ui_id_edit)->MoveWindow(&rect);
					}
				}
			}
		private:
			BEGIN_MESSAGE_MAP__IN_CLASS(Dialog_MultiLinesEditor)
				ON_WM_SIZE()
				ON_BN_CLICKED(m_ui_id_button_ok, on_button_ok__clicked)
				ON_BN_CLICKED(m_ui_id_button_cancel, on_button_cancel__clicked)
			END_MESSAGE_MAP__IN_CLASS()
#pragma endregion
		};
	}

	//从用户获得一些数据的对话框
	//Args: 参数的类型
	template<typename... Args>
	class ArgsDialog : public CDialog
	{
#pragma region ui界面布局常量
	private:
		static const short m_ui_width_name = _2022_3_13_fix(80);
		static const short m_ui_width_interval_behind_name = _2022_3_13_fix(5);
		static const short m_ui_width_button = _2022_3_13_fix(48);
		static const short m_ui_width_edit_int = _2022_3_13_fix(100);
		static const short m_ui_width_edit_float = _2022_3_13_fix(100);
		static const short m_ui_width_combo_box = _2022_3_13_fix(100);
		static const short m_ui_width_button_string = m_ui_width_button;
		static const short m_ui_width_edit_string = _2022_3_13_fix(100);
		static const short m_ui_width_interval_behind_edit_int = _2022_3_13_fix(16);
		static const short m_ui_width_interval_behind_edit_float = _2022_3_13_fix(16);
		static const short m_ui_width_interval_behind_checkbox_bool = _2022_3_13_fix(16);
		static const short m_ui_width_interval_behind_ctrl_for_string = _2022_3_13_fix(16);
		static const short m_ui_width_int_type_desc = _2022_3_13_fix(100);
		static const short m_ui_width_float_type_desc = _2022_3_13_fix(100);
		static const short m_ui_width_bool_type_desc = _2022_3_13_fix(100);
		static const short m_ui_width_string_type_desc = _2022_3_13_fix(100);
		static const short m_ui_width_checkbox_bool = (int)(8*1.5 + 0.5);
		static const short m_ui_width_before_ok_button = _2022_3_13_fix(170);		//在ok按钮前边额外的空白
		static const short m_ui_width_before_cancel_button = _2022_3_13_fix(10);	//在cancel按钮前边额外的空白
		static const short m_ui_height = _2022_3_13_fix(17);
		static const short m_ui_height_combo_box = (int)(200*1.5 + 0.5);	//下拉列表的高度, 包括下拉表
		static const short m_ui_height_line_interval = _2022_3_13_fix(3);
		static const short m_ui_height_button = _2022_3_13_fix(17);
		static const short m_ui_height_interval_above_ok_button = _2022_3_13_fix(13);	//在ok按钮上边额外的空白
#pragma endregion
#pragma region 父窗口
	private:
		CWnd* m_parent{};
	public:
		void set_parent(CWnd* parent)
		{
			m_parent = parent;
		}
#pragma endregion

#pragma region 构建对话框
	public:
		INT_PTR DoModal() override
		{
			//指定模板
			set_template();

			return __super::DoModal();
		}
	private:
		struct s_fn_make_template
		{
			//设置一行的变量名和默认值
			template<std::size_t index>
			static void call(_In_ ArgsDialog* _this,
				_Inout_ IdGiver<WORD> & id_giver, _Inout_ short & y)
			{
				std::get<index>(_this->m_arg_infos).build_ui(_this->m_template, y, y, id_giver, _this);
			};
		};
		void set_template()
		{
			if (m_is_template_setted)
				return;

			//创建模板
			{
				//初始化
				short y{};
				IdGiver<WORD> id_giver{m_ui_id_first, m_ui_id_max};

				//循环写入每一个变量需要的控件
				iterator_arg_all_index<s_fn_make_template>(id_giver, y);

				//分配控件id
//					id_giver.get_one_id(m_ui_button_id_ok);
//					id_giver.get_one_id(m_ui_button_id_cancle);

				//写入两个按钮
				y += m_ui_height_interval_above_ok_button;
				using Control = utils::windows::mfc::DialogTemplate::Control;
				m_template.add_one_item(DLGITEMTEMPLATE{BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE, 0,  m_ui_width_before_ok_button, y, m_ui_width_button, m_ui_height_button, m_ui_button_id_ok}, Control::button, "确定");
				m_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD | WS_VISIBLE, 0,  m_ui_width_before_ok_button + m_ui_width_button + m_ui_width_before_cancel_button, y, m_ui_width_button, m_ui_height_button, m_ui_button_id_cancel}, Control::button, "取消");
				y += m_ui_height_button;
				y += m_ui_height_line_interval;

				//写入对话框的基本信息
				m_template.set(DLGTEMPLATE{WS_POPUP | WS_CAPTION | WS_VISIBLE, 0, 0, 0, 0, _2022_3_13_fix(300), y}, "参数设置");
			}

			//使用模板
			m_template.use_to(this, m_parent);

			//更新状态
			m_is_template_setted = true;
		}
		bool m_is_template_setted{};
		utils::windows::mfc::DialogTemplate m_template;

	private:
		struct s_fn_init_dialog
		{
			//通知每个参数的子控件 OnInitDialog
			template<std::size_t index>
			static void call(_In_ ArgsDialog* _this)
			{
				std::get<index>(_this->m_arg_infos).on_init_dialog();
			};
		};
		//获得控件指针
		template<typename ctrl_T>
		ctrl_T* get_ctrl(_In_ const WORD id)
		{
			auto ctrl = (ctrl_T*)GetDlgItem(id);
			utils_assert(ctrl);
			return ctrl;
		}
		BOOL OnInitDialog() override
		{
			__super::OnInitDialog();

			//todo 检查是否所有的参数都设置了

			//通知每个参数的子控件
			iterator_arg_all_index<s_fn_init_dialog>();

			//设置字体
			using namespace utils::windows::mfc::warehouse::import;
			warehouse::font::SetFont(*get_ctrl<CButton>(m_ui_button_id_ok), warehouse::font::Id::Button);
			warehouse::font::SetFont(*get_ctrl<CButton>(m_ui_button_id_cancel), warehouse::font::Id::Button);

			return TRUE;  // return TRUE unless you set the focus to a control
							// 异常: OCX 属性页应返回 FALSE
		}
#pragma endregion

#pragma region 参数及其设置
#pragma region s_arg_info的定义
	public:
#pragma region s_specific_arg_info的定义
		//特定类型能用到的设置
		//basic_T: 父类
		//info_T: s_type_info<,,,> 用于返回*this时强制转换
		//arg_T: 参数类型
		template<typename basic_T, typename info_T, typename arg_T>
		struct s_specific_arg_info : public basic_T
		{
		};
#pragma region s_specific_arg_info的特化: 整数
		//特化 int的公共类
		template<typename basic_T, typename info_T>
		struct s_specific_arg_info__int : public basic_T
		{
		public:
			bool m_is_hex{};			//是否十六进制显示

		public:
			info_T & set_is_hex(const bool is_hex)
			{
				m_is_hex = is_hex;
				return *(info_T*)this;
			}
			//从控件里获得值
			bool try_get_value(_Out_ typename basic_T::type & value)
			{
				//获得控件指针
				auto ui_edit = (CEdit*)m_dlg->GetDlgItem(m_ui_edit_id);
				utils_assert(ui_edit);

				//获得里边的字符串
				CString text;
				ui_edit->GetWindowTextA(text);

				//转为整数
				if (false == utils::string_function::format_number::try_to_integer(value, text.GetString(), m_is_hex))
				{
					std::string type_name;
					utils::string_function::get_type_name<typename basic_T::type>(type_name);
					m_dlg->MessageBox(CString() + "请输入一个 " + type_name.c_str());
					return false;
				}

				//做用户自定义的检查
				if (m_check_func)
				{
					if (false == m_check_func(value))
						return false;
				}
				return true;
			}
			std::string get_type_desc()
			{
				//类型
				std::string desc;
				utils::string_function::get_type_name<typename basic_T::type>(desc);

				//进制
				if (m_is_hex)
					desc += "  十六进制";
				return desc;
			}
			std::string make_value_text()
			{
				utils::string text;
				text.format(utils::string_function::format_number::get_placeholder_of_printf<typename basic_T::type>(m_is_hex).c_str(), this->m_value);
				return text;
			}
#if 0
			//获得控件指针
			template<typename ctrl_T>
			ctrl_T* get_ctrl(_In_ const WORD id)
			{
				auto ctrl = (ctrl_T*)m_dlg->GetDlgItem(id);
				utils_assert(ctrl);
				return ctrl;
			}
#endif
			void on_init_dialog()
			{
				//更新值 (因为m_value可能和dlg_template里的不一样)
				get_ctrl<CEdit>(m_ui_edit_id)->SetWindowTextA(make_value_text().c_str());

				//设置字体
				using namespace utils::windows::mfc::warehouse::import;
				warehouse::font::SetFont(*get_ctrl<CEdit>(m_ui_edit_id), warehouse::font::Id::Edit);
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_name), warehouse::font::Id::Static_EditName);
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_desc), warehouse::font::Id::Static_EditDesc);
			}
			void _build_ui(_Inout_ utils::windows::mfc::DialogTemplate & dlg_template, _Out_ short & next_y, _In_ const short _y, _Inout_ IdGiver<WORD> & id_giver)
			{
				//分配控件id
				id_giver.get_one_id(m_ui_static_id_name);
				id_giver.get_one_id(m_ui_static_id_desc);
				id_giver.get_one_id(m_ui_edit_id);

				//写入对话框模板
				using Control = utils::windows::mfc::DialogTemplate::Control;
				short x = 0;
				short y = _y + m_ui_height_line_interval;
				{
					//要显示的字符串
					auto _name = this->m_name + "：";

					//需要的宽度
					short width{};
					using namespace utils::windows::mfc::warehouse::import;
					get_width(width, *warehouse::font::get(warehouse::font::Id::Static_EditName), _name);
					if (width < m_ui_width_name)
						width = m_ui_width_name;

					//写入
					dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_RIGHT | WS_CHILD | WS_VISIBLE, 0, x, y, width, m_ui_height, m_ui_static_id_name}, Control::_static, _name);
//						dlg_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD | WS_VISIBLE, 0, x, y, width, m_ui_height, 0}, Control::button, _name);
					x += width + m_ui_width_interval_behind_name;
				}
				dlg_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD | WS_BORDER | WS_VISIBLE, 0, x, y, m_ui_width_edit_int, m_ui_height, m_ui_edit_id}, Control::edit, make_value_text());
				x += m_ui_width_edit_int + m_ui_width_interval_behind_edit_int;
				dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_LEFT | WS_CHILD | WS_VISIBLE, 0, x, y, m_ui_width_int_type_desc, m_ui_height, m_ui_static_id_desc}, Control::_static, get_type_desc());

				//添加控件的消息处理函数
				m_dlg->add_ctrl_command_handler(m_ui_edit_id, [this](_In_ const WORD notify_code)
				{
					switch(notify_code)
					{
					case EN_KILLFOCUS:
					{
						//此函数块全局互斥
						if (m_dlg->m_is_checking)
							break;
						m_dlg->m_is_checking = true;

						//检查当前输入的数据，如果数据不正确，把焦点再转回来

						typename basic_T::type value{};
						if (false == try_get_value(value))
						{
							//获得控件指针
							auto ui_edit = (CEdit*)m_dlg->GetDlgItem(m_ui_edit_id);
							utils_assert(ui_edit);

							//设置焦点
							ui_edit->SetFocus();
						}
						else
						{
							//保存
							this->m_value = value;
						}

						//互斥结束
						m_dlg->m_is_checking = false;
						break;
					}
					}
				});

				//更新状态
				next_y = y + m_ui_height;
			}
		private:
			WORD m_ui_edit_id{};
			WORD m_ui_static_id_name{};
			WORD m_ui_static_id_desc{};
		};
		//特化 int
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, int8_t>		: s_specific_arg_info__int<basic_T, info_T> {};
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, int16_t>	: s_specific_arg_info__int<basic_T, info_T> {};
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, int32_t>	: s_specific_arg_info__int<basic_T, info_T> {};
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, int64_t>	: s_specific_arg_info__int<basic_T, info_T> {};
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, uint8_t>	: s_specific_arg_info__int<basic_T, info_T> {};
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, uint16_t>	: s_specific_arg_info__int<basic_T, info_T> {};
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, uint32_t>	: s_specific_arg_info__int<basic_T, info_T> {};
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, uint64_t>	: s_specific_arg_info__int<basic_T, info_T> {};
#pragma endregion
#pragma region s_specific_arg_info的特化: 浮点数
		//特化 浮点数的公共类
		template<typename basic_T, typename info_T>
		struct s_specific_arg_info__float : public basic_T
		{
		public:
			//从控件里获得值
			bool try_get_value(_Out_ typename basic_T::type & value)
			{
				//获得控件指针
				auto ui_edit = (CEdit*)m_dlg->GetDlgItem(m_ui_edit_id);
				utils_assert(ui_edit);

				//获得里边的字符串
				CString text;
				ui_edit->GetWindowTextA(text);

				//转为浮点数
				if (utils_fail(utils::string_function::to_number(value, text.GetString())))
				{
					std::string type_name;
					utils::string_function::get_type_name<typename basic_T::type>(type_name);
					m_dlg->MessageBoxA(CString() + "请输入一个 " + type_name.c_str());
					return false;
				}

				//做用户自定义的检查
				if (m_check_func)
				{
					if (false == m_check_func(value))
						return false;
				}
				return true;
			}
			std::string get_type_desc()
			{
				//类型
				std::string desc;
				utils::string_function::get_type_name<typename basic_T::type>(desc);

//				//进制
//				if (m_is_hex)
//					desc += "  十六进制";
				return desc;
			}
			std::string make_value_text()
			{
				utils::string text;
				text.format(utils::string_function::format_number::get_placeholder_of_printf<typename basic_T::type>().c_str(), this->m_value);
				return text;
			}
			void on_init_dialog()
			{
				//更新值 (因为m_value可能和dlg_template里的不一样)
				get_ctrl<CEdit>(m_ui_edit_id)->SetWindowTextA(make_value_text().c_str());

				//设置字体
				using namespace utils::windows::mfc::warehouse::import;
				warehouse::font::SetFont(*get_ctrl<CEdit>(m_ui_edit_id), warehouse::font::Id::Edit);
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_name), warehouse::font::Id::Static_EditName);
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_desc), warehouse::font::Id::Static_EditDesc);
			}
			void _build_ui(_Inout_ utils::windows::mfc::DialogTemplate & dlg_template, _Out_ short & next_y, _In_ const short _y, _Inout_ IdGiver<WORD> & id_giver)
			{
				//分配控件id
				id_giver.get_one_id(m_ui_static_id_name);
				id_giver.get_one_id(m_ui_static_id_desc);
				id_giver.get_one_id(m_ui_edit_id);

				//写入对话框模板
				using Control = utils::windows::mfc::DialogTemplate::Control;
				short x = 0;
				short y = _y + m_ui_height_line_interval;
				{
					//要显示的字符串
					auto _name = this->m_name + "：";

					//需要的宽度
					short width{};
					using namespace utils::windows::mfc::warehouse::import;
					get_width(width, *warehouse::font::get(warehouse::font::Id::Static_EditName), _name);
					if (width < m_ui_width_name)
						width = m_ui_width_name;

					//写入
					dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_RIGHT | WS_CHILD | WS_VISIBLE, 0, x, y, width, m_ui_height, m_ui_static_id_name}, Control::_static, _name);
					x += width + m_ui_width_interval_behind_name;
				}
				dlg_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD | WS_BORDER | WS_VISIBLE, 0, x, y, m_ui_width_edit_float, m_ui_height, m_ui_edit_id}, Control::edit, make_value_text());
				x += m_ui_width_edit_float + m_ui_width_interval_behind_edit_float;
				dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_LEFT | WS_CHILD | WS_VISIBLE, 0, x, y, m_ui_width_float_type_desc, m_ui_height, m_ui_static_id_desc}, Control::_static, get_type_desc());

				//添加控件的消息处理函数
				m_dlg->add_ctrl_command_handler(m_ui_edit_id, [this](_In_ const WORD notify_code)
				{
					switch(notify_code)
					{
					case EN_KILLFOCUS:
					{
						//此函数块全局互斥
						if (m_dlg->m_is_checking)
							break;
						m_dlg->m_is_checking = true;

						//检查当前输入的数据，如果数据不正确，把焦点再转回来

						typename basic_T::type value{};
						if (false == try_get_value(value))
						{
							//获得控件指针
							auto ui_edit = (CEdit*)m_dlg->GetDlgItem(m_ui_edit_id);
							utils_assert(ui_edit);

							//设置焦点
							ui_edit->SetFocus();
						}
						else
						{
							//保存
							this->m_value = value;
						}

						//互斥结束
						m_dlg->m_is_checking = false;
						break;
					}
					}
				});

				//更新状态
				next_y = y + m_ui_height;
			}
		private:
			WORD m_ui_edit_id{};
			WORD m_ui_static_id_name{};
			WORD m_ui_static_id_desc{};
		};
		//特化 float
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, float>	: s_specific_arg_info__float<basic_T, info_T> {};
		template<typename basic_T, typename info_T> struct s_specific_arg_info<basic_T, info_T, double>	: s_specific_arg_info__float<basic_T, info_T> {};

#pragma endregion
#pragma region s_specific_arg_info的特化: bool
		//特化 bool
		template<typename basic_T, typename info_T>
		struct s_specific_arg_info<basic_T, info_T, bool> : public basic_T
		{
		public:
			//从控件里获得值
			bool try_get_value(_Out_ typename basic_T::type & value)
			{
				//获得控件指针
				auto ui_checkbox = (CButton*)m_dlg->GetDlgItem(m_ui_checkbox_id);
				utils_assert(ui_checkbox);

				//获得选中状态
				value = ui_checkbox->GetCheck() == BST_CHECKED;

				//做用户自定义的检查
				if (m_check_func)
				{
					if (false == m_check_func(value))
						return false;
				}
				return true;
			}
			std::string get_type_desc()
			{
				return "";
				//类型
				std::string desc;
				utils::string_function::get_type_name<typename basic_T::type>(desc);
				return desc;
			}
			void on_init_dialog()
			{
				//更新值 (因为m_value可能和dlg_template里的不一样)
				get_ctrl<CButton>(m_ui_checkbox_id)->SetCheck(this->m_value ? BST_CHECKED : BST_UNCHECKED);

				//设置字体
				using namespace utils::windows::mfc::warehouse::import;
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_name), warehouse::font::Id::Static_EditName);
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_desc), warehouse::font::Id::Static_EditDesc);
			}
			void _build_ui(_Inout_ utils::windows::mfc::DialogTemplate & dlg_template, _Out_ short & next_y, _In_ const short _y, _Inout_ IdGiver<WORD> & id_giver)
			{
				//分配控件id
				id_giver.get_one_id(m_ui_static_id_name);
				id_giver.get_one_id(m_ui_static_id_desc);
				id_giver.get_one_id(m_ui_checkbox_id);

				//写入对话框模板
				using Control = utils::windows::mfc::DialogTemplate::Control;
				short x = 0;
				short y = _y + m_ui_height_line_interval;
				{
					//要显示的字符串
					auto _name = this->m_name + "：";

					//需要的宽度
					short width{};
					using namespace utils::windows::mfc::warehouse::import;
					get_width(width, *warehouse::font::get(warehouse::font::Id::Static_EditName), _name);
					if (width < m_ui_width_name)
						width = m_ui_width_name;

					//写入
					dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_RIGHT | WS_CHILD | WS_VISIBLE, 0, x, y, width, m_ui_height, m_ui_static_id_name}, Control::_static, _name);
					x += width + m_ui_width_interval_behind_name;
				}
				dlg_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD/* | WS_BORDER */| WS_VISIBLE | BS_AUTOCHECKBOX | BS_NOTIFY, 0, x, y, m_ui_width_checkbox_bool, m_ui_height, m_ui_checkbox_id}, Control::button, "");
				x += m_ui_width_checkbox_bool + m_ui_width_interval_behind_checkbox_bool;
				dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_LEFT | WS_CHILD | WS_VISIBLE, 0, x, y, m_ui_width_bool_type_desc, m_ui_height, m_ui_static_id_desc}, Control::_static, get_type_desc());

				//添加控件的消息处理函数
				m_dlg->add_ctrl_command_handler(m_ui_checkbox_id, [this](_In_ const WORD notify_code)
				{
					switch(notify_code)
					{
					case BN_KILLFOCUS:
					{
						//此函数块全局互斥
						if (m_dlg->m_is_checking)
							break;
						m_dlg->m_is_checking = true;

						//检查当前输入的数据，如果数据不正确，把焦点再转回来

						typename basic_T::type value{};
						if (false == try_get_value(value))
						{
							//获得控件指针
							auto ui_checkbox = (CButton*)m_dlg->GetDlgItem(m_ui_checkbox_id);
							utils_assert(ui_checkbox);

							//设置焦点
							ui_checkbox->SetFocus();
						}
						else
						{
							//保存
							this->m_value = value;
						}

						//互斥结束
						m_dlg->m_is_checking = false;
						break;
					}
					}
				});

				//更新状态
				next_y = y + m_ui_height;
			}
		private:
			WORD m_ui_checkbox_id{};
			WORD m_ui_static_id_name{};
			WORD m_ui_static_id_desc{};
		};
#pragma endregion
#pragma region s_specific_arg_info的特化: 下拉列表
		//特化 下拉列表
		//item_T: 用户传入的参数的类型, 在utils里不用
		template<typename basic_T, typename info_T, typename item_T>
		struct s_specific_arg_info<basic_T, info_T, std::tuple<std::string, item_T>> : public basic_T
		{
		public:
			std::vector<std::tuple<std::string, item_T>> m_choices;
			uint64_t m_current_index{};
		public:
			//设置下拉列表里的选项
			info_T & set_choices(_In_ const std::vector<std::tuple<std::string, item_T>> & choices)
			{
				m_choices = choices;
				return *(info_T*)this;
			}
			//设置当前选择项
			info_T & set_current_index(_In_ const uint64_t index)
			{
				//简单检查
				utils_assert(index < m_choices.size());

				m_current_index = index;
				return *(info_T*)this;
			}
			//从控件里获得值(以索引返回)
			bool try_get_value(_Out_ uint64_t & value_index)
			{
				//获得控件指针
				auto ui_combo_box = get_ctrl<CComboBox>(m_ui_combo_box_id);

				//当前的选择
				auto cur_sel = ui_combo_box->GetCurSel();
				utils_assert(cur_sel != CB_ERR && cur_sel >= 0 && (uint64_t)cur_sel < m_choices.size());
				value_index = cur_sel;

				//做用户自定义的检查
				if (m_check_func)
				{
					if (false == m_check_func(m_choices[value_index]))
						return false;
				}
				return true;
			}
			void on_init_dialog()
			{
				//更新值 (因为m_value可能和dlg_template里的不一样)
				{
					//设置下拉列表的内容

					//获得控件指针
					auto ui_combo_box = get_ctrl<CComboBox>(m_ui_combo_box_id);

					//清空
					ui_combo_box->ResetContent();

					//简单检查
					utils_assert(false == m_choices.empty());

					//设置内容
					for (const auto & choice : m_choices)
					{
						const auto & text = std::get<0>(choice);
						utils_assert(0 <= ui_combo_box->AddString(text.c_str()));
					}

					auto tmp = ui_combo_box->GetCount();

					//设置当前选项
					utils_assert(CB_ERR != ui_combo_box->SetCurSel((int)m_current_index));	//todo check (int) cast

					//设置下拉表的宽度
//					utils_assert(CB_ERR != ui_combo_box->SetDroppedWidth());
				}

				//设置字体
				using namespace utils::windows::mfc::warehouse::import;
				warehouse::font::SetFont(*get_ctrl<CComboBox>(m_ui_combo_box_id), warehouse::font::Id::ComboBox);
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_name), warehouse::font::Id::Static_EditName);
//				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_desc), warehouse::font::Id::Static_EditDesc);
			}
			void _build_ui(_Inout_ utils::windows::mfc::DialogTemplate & dlg_template, _Out_ short & next_y, _In_ const short _y, _Inout_ IdGiver<WORD> & id_giver)
			{
				//分配控件id
				id_giver.get_one_id(m_ui_static_id_name);
//				id_giver.get_one_id(m_ui_static_id_desc);
				id_giver.get_one_id(m_ui_combo_box_id);

				//写入对话框模板
				using Control = utils::windows::mfc::DialogTemplate::Control;
				short x = 0;
				short y = _y + m_ui_height_line_interval;
				{
					//要显示的字符串
					auto _name = this->m_name + "：";

					//需要的宽度
					short width{};
					using namespace utils::windows::mfc::warehouse::import;
					get_width(width, *warehouse::font::get(warehouse::font::Id::Static_EditName), _name);
					if (width < m_ui_width_name)
						width = m_ui_width_name;

					//写入
					dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_RIGHT | WS_CHILD | WS_VISIBLE, 0, x, y, width, m_ui_height, m_ui_static_id_name}, Control::_static, _name);
					x += width + m_ui_width_interval_behind_name;
				}
				dlg_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD/* | WS_BORDER */| WS_VISIBLE | CBS_DROPDOWNLIST /* | WS_VSCROLL | WS_TABSTOP*/, 0, x, y, m_ui_width_combo_box, m_ui_height_combo_box, m_ui_combo_box_id}, Control::combo_box, "");
//				x += m_ui_width_combo_box + m_ui_width_interval_behind_combo_box;
//				dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_LEFT | WS_CHILD | WS_VISIBLE, 0, x, y, m_ui_width_bool_type_desc, m_ui_height, m_ui_static_id_desc}, Control::_static, get_type_desc());

				//添加控件的消息处理函数
				m_dlg->add_ctrl_command_handler(m_ui_combo_box_id, [this](_In_ const WORD notify_code)
				{
					switch(notify_code)
					{
					case CBN_KILLFOCUS:
					{
						//此函数块全局互斥
						if (m_dlg->m_is_checking)
							break;
						m_dlg->m_is_checking = true;

						//检查当前输入的数据，如果数据不正确，把焦点再转回来

						uint64_t value_index{};
						if (false == try_get_value(value_index))
						{
							//设置焦点
							get_ctrl<CComboBox>(m_ui_combo_box_id)->SetFocus();
						}
						else
						{
							//保存
							m_current_index = value_index;
							m_value = m_choices[m_current_index];	//todo: 去掉此类型里的m_value
						}

						//互斥结束
						m_dlg->m_is_checking = false;
						break;
					}
					}
				});

				//更新状态
				next_y = y + m_ui_height;
			}
		private:
			WORD m_ui_combo_box_id{};
			WORD m_ui_static_id_name{};
//			WORD m_ui_static_id_desc{};
		};
#pragma endregion
#pragma region s_specific_arg_info的特化: 字符串
		//特化: 字符串
		template<typename basic_T, typename info_T>
		struct s_specific_arg_info<basic_T, info_T, std::string> : public basic_T
		{
		public:
			bool m_is_multi_line{};
			std::string m_text_multi_line;

		public:
			void set_is_multi_line(_In_ const bool is_multi_line)
			{
				m_is_multi_line = is_multi_line;
			}
			//从控件里获得值
			bool try_get_value(_Out_ typename basic_T::type & value)
			{
				//获得值
				if (m_is_multi_line)
				{
					value = m_text_multi_line;
				}
				else
				{
					CString text;
					get_ctrl<CEdit>(m_ui_edit_id)->GetWindowTextA(text);
					value = text.GetString();

					//简单检查
					utils_assert(value.find('\n') == std::string::npos);
				}

				//做用户自定义的检查
				if (m_check_func)
				{
					if (false == m_check_func(value))
						return false;
				}
				return true;
			}
			void on_init_dialog()
			{
				//更新值 (因为m_value可能和dlg_template里的不一样)
				if (m_is_multi_line)
				{
					m_text_multi_line = m_value;
				}
				else
				{
					get_ctrl<CEdit>(m_ui_edit_id)->SetWindowTextA(m_value.c_str());
				}

				//设置字体
				using namespace utils::windows::mfc::warehouse::import;
				if (m_is_multi_line)
				{
					warehouse::font::SetFont(*get_ctrl<CButton>(m_ui_button_id), warehouse::font::Id::Button);
				}
				else
				{
					warehouse::font::SetFont(*get_ctrl<CEdit>(m_ui_edit_id), warehouse::font::Id::Edit);
				}
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_name), warehouse::font::Id::Static_EditName);
				warehouse::font::SetFont(*get_ctrl<CStatic>(m_ui_static_id_desc), warehouse::font::Id::Static_EditDesc);
			}
			std::string get_type_desc()
			{
				if (m_is_multi_line)
					return "std::string  多行";
				else
					return "std::string  单行";
			}
			void _build_ui(_Inout_ utils::windows::mfc::DialogTemplate & dlg_template, _Out_ short & next_y, _In_ const short _y, _Inout_ IdGiver<WORD> & id_giver)
			{
				//分配控件id
				id_giver.get_one_id(m_ui_static_id_name);
				id_giver.get_one_id(m_ui_static_id_desc);
				id_giver.get_one_id(m_ui_edit_id);
				id_giver.get_one_id(m_ui_button_id);

				//写入对话框模板
				using Control = utils::windows::mfc::DialogTemplate::Control;
				short x = 0;
				short y = _y + m_ui_height_line_interval;
				{
					//要显示的字符串
					auto _name = this->m_name + "：";

					//需要的宽度
					short width{};
					using namespace utils::windows::mfc::warehouse::import;
					get_width(width, *warehouse::font::get(warehouse::font::Id::Static_EditName), _name);
					if (width < m_ui_width_name)
						width = m_ui_width_name;

					//写入
					dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_RIGHT | WS_CHILD | WS_VISIBLE, 0, x, y, width, m_ui_height, m_ui_static_id_name}, Control::_static, _name);
					x += width + m_ui_width_interval_behind_name;
				}
				WORD ui_width_ctrl{};
				if (m_is_multi_line)
				{
					dlg_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD | WS_VISIBLE, 0, x, y, m_ui_width_button_string, m_ui_height, m_ui_button_id}, Control::button, "编辑...");
					ui_width_ctrl = m_ui_width_button_string;
				}
				else
				{
					dlg_template.add_one_item(DLGITEMTEMPLATE{WS_CHILD | WS_BORDER | WS_VISIBLE, 0, x, y, m_ui_width_edit_string, m_ui_height, m_ui_edit_id}, Control::edit, m_value);
					ui_width_ctrl = m_ui_width_edit_string;
				}
				x += ui_width_ctrl + m_ui_width_interval_behind_ctrl_for_string;
				dlg_template.add_one_item(DLGITEMTEMPLATE{SS_CENTERIMAGE | SS_LEFT | WS_CHILD | WS_VISIBLE, 0, x, y, m_ui_width_string_type_desc, m_ui_height, m_ui_static_id_desc}, Control::_static, get_type_desc());

				//添加控件的消息处理函数
				m_dlg->add_ctrl_command_handler(m_ui_edit_id, [this](_In_ const WORD notify_code)
				{
					switch(notify_code)
					{
					case EN_KILLFOCUS:
					{
						//简单检查
						utils_assert(false == m_is_multi_line);
						
						//此函数块全局互斥
						if (m_dlg->m_is_checking)
							break;
						m_dlg->m_is_checking = true;

						//检查当前输入的数据，如果数据不正确，把焦点再转回来

						typename basic_T::type value{};
						if (false == try_get_value(value))
						{
							//设置焦点
							get_ctrl<CEdit>(m_ui_edit_id)->SetFocus();
						}
						else
						{
							//保存
							m_value = value;
						}

						//互斥结束
						m_dlg->m_is_checking = false;
						break;
					}
					}
				});
				m_dlg->add_ctrl_command_handler(m_ui_button_id, [this](_In_ const WORD notify_code)
				{
					switch(notify_code)
					{
					case BN_CLICKED:
					{
						//简单检查
						utils_assert(m_is_multi_line);
						
						//显示个多行编辑窗口, 获取多行的字符串
						inner::Dialog_MultiLinesEditor dlg;
						dlg.set_parent(m_dlg);
						dlg.set_check_func(m_check_func);
						dlg.set_text(m_value);
						if (dlg.DoModal() == IDOK)
							dlg.get_text(m_value);
#if 0
						
						//此函数块全局互斥
						if (m_dlg->m_is_checking)
							break;
						m_dlg->m_is_checking = true;

						//检查当前输入的数据，如果数据不正确，把焦点再转回来

						typename basic_T::type value{};
						if (false == try_get_value(value))
						{
							//设置焦点
							get_ctrl<CEdit>(m_ui_edit_id)->SetFocus();
						}
						else
						{
							//保存
							m_value = value;
						}

						//互斥结束
						m_dlg->m_is_checking = false;
#endif
						break;
					}
					}
				});

				//更新状态
				next_y = y + m_ui_height;
			}
		private:
			WORD m_ui_edit_id{};
			WORD m_ui_button_id{};
			WORD m_ui_static_id_name{};
			WORD m_ui_static_id_desc{};
		};
#pragma endregion
#pragma endregion
#pragma region s_basic_arg_info的定义
		//所有类型都能用到的设置
		template<typename info_T, typename arg_T>
		struct s_basic_arg_info
		{
		public:
			using type = arg_T;		//参数的类型
			using check_func_t = std::function<bool(_In_ const arg_T value)>;		//检查函数的类型	//todo 参数改为引用

		public:
			std::string m_name;		//名字
			arg_T m_value{};		//当前值
			arg_T m_old_value{};	//原值
			check_func_t m_check_func;	//检查函数
				
		protected:
			ArgsDialog* m_dlg{};

		public:
			info_T & set_name(_In_ const std::string & name)
			{
				m_name = name;
				return *(info_T*)this;
			}
//				s_arg_info & set_default_value(_In_ const arg_T & value)
//				{
//					this->default_value = value;
//					return *this;
//				}
			info_T & set_value(_In_ const arg_T & value)
			{
				m_value = value;
				m_old_value = value;
				return *(info_T*)this;
			}
			info_T & set_check_func(_In_ check_func_t check_func)
			{
				m_check_func = check_func;
				return *(info_T*)this;
			}
			const arg_T & get_value() const
			{
				return m_value;
			}
			void on_ok()
			{
				//保存当前值
				m_old_value = m_value;
			}
			void on_cancel()
			{
				//恢复原有值
				m_value = m_old_value;
			}
			void build_ui(_Inout_ utils::windows::mfc::DialogTemplate & dlg_template, _Out_ short & next_y, _In_ const short _y, _Inout_ IdGiver<WORD> & id_giver, _In_ ArgsDialog* dlg)
			{
				//保存
				utils_assert(dlg);
				m_dlg = dlg;

				return ((info_T*)this)->_build_ui(dlg_template, next_y, _y, id_giver);
			}
		protected:
			//获得控件指针
			template<typename ctrl_T>
			ctrl_T* get_ctrl(_In_ const WORD id)
			{
				auto ctrl = (ctrl_T*)this->m_dlg->GetDlgItem(id);
				utils_assert(ctrl);
				return ctrl;
			}
			//获取指定字符串在指定字体时的像素长度
			void get_width(_Out_ short & width, _In_ CFont & font, _In_ const std::string & text)
			{
				CDC dc;
				utils_assert(dc.CreateCompatibleDC(NULL));
				auto old_font = dc.SelectObject(&font);
				auto size = dc.GetTextExtent(text.c_str());
				width = (short)size.cx;
				dc.SelectObject(old_font);
			}
		};
#pragma endregion
		//类型能用到的设置
		template<typename arg_T>
		struct s_arg_info : public s_specific_arg_info<s_basic_arg_info<s_arg_info<arg_T>, arg_T>, s_arg_info<arg_T>, arg_T>
		{
		};
#pragma endregion

	public:
//			//设置指定位置的参数的默认值和检查函数
//			template<std::size_t index, typename func_T>
//			void SetArg(
//				_In_ const std::string & name,
//				_In_ const get_arg_t<index> & default_value,
//				_In_ const func_T & check)
//			{
//				//保存变量名
//				m_names[index] = name;
//
//				//保存默认值
//				std::get<index>(m_default_values) = default_value;
//
//				//保存检查函数
//				m_check_funcs[index] = [this, check]() -> bool	//check按值传入
//				{
//					return check(get_value<index>());
//				};
//			}
		//获取指定位置的参数的设置集 (所说的设置集也就是s_arg_info)
		template<std::size_t index>
		auto & ArgInfo()
		{
			return std::get<index>(m_arg_infos);
		}
		//获得所有参数
		std::tuple<Args...> GetArgs()
		{
			return _GetArgs(std::make_index_sequence<sizeof...(Args)>{});
		}
	private:
		template <std::size_t... I>
		std::tuple<Args...> _GetArgs(std::index_sequence<I...>)
		{
			return std::tuple<Args...>{
				ArgInfo<I>().get_value()...
			};
		}
		//遍历每个变量，调用指定函数
		template<typename s_fn_T, typename ... _Args>
		void iterator_arg_all_index(_Args && ... args)
		{
			//实现类似功能:
			//	s_fn_T::call<0>(this);
			//	s_fn_T::call<1>(this);
			//	...
			//	s_fn_T::call<sizeof...(Args) - 1>(this);

			utils::iterator::s_iterator_index<sizeof...(Args), s_fn_T>::iterator(this, std::forward<_Args>(args)...);
		}

		std::tuple<s_arg_info<Args>...> m_arg_infos;
#pragma endregion
			
#pragma region 控件id
	private:
		static const WORD m_ui_button_id_ok = 5000;
		static const WORD m_ui_button_id_cancel = 5001;
		static const WORD m_ui_id_first = 10000;
		static const WORD m_ui_id_max = 20000;
#pragma endregion

#pragma region 消息处理
	private:
		using ctrl_command_handler_t = std::function<void(_In_ const WORD notify_code)>;
		utils::map<WORD /*控件id*/ , ctrl_command_handler_t> m_ctrl_command_handlers;
	private:
		void add_ctrl_command_handler(_In_ const WORD id, _In_ ctrl_command_handler_t handler)
		{
			//简单检查
			utils_assert(id >= m_ui_id_first && id <= m_ui_id_max);

			m_ctrl_command_handlers.insert_not_exist(id, handler);
		}
		BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override
		{
			//转发给自己的控件
			if (nCode >= 0)	//todo  有时nCode会为负数
			{
				//准备数据
				WORD _id{};
				WORD notify_code{};
				{
					utils::computer_basic_function::store_this_integer(_id, nID);
//					auto msg = GetCurrentMessage();
//					notify_code = HIWORD(msg->wParam);
					utils::computer_basic_function::store_this_integer(notify_code, nCode);
				}

				//转发
				if (_id >= m_ui_id_first && _id <= m_ui_id_max)
				{
					m_ctrl_command_handlers.get_existed_value(_id)(notify_code);
					return TRUE;
				}
			}

			//默认处理
			return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
		}
		void OnCtrlCommand(UINT id)
		{
			//准备数据
			WORD _id{};
			WORD notify_code{};
			{
				utils::computer_basic_function::store_this_integer(_id, id);
				auto msg = GetCurrentMessage();
				notify_code = HIWORD(msg->wParam);
			}

			//转发
			m_ctrl_command_handlers.get_existed_value(_id)(notify_code);
		}
		void OnButtonClicked_OK()
		{
//				if (IDOK != m_dlg.DoModal())
//					return;

//			utils::call_func(m_func, m_dlg.GetArgs());
		}
		void OnButtonClicked_Cancel()
		{
		}
		void OnSize(UINT nType, int cx, int cy)
		{
			__super::OnSize(nType, cx, cy);

			//放置按钮

		}
		struct s_fn_on_ok
		{
			//通知一行的变量 OnOK
			template<std::size_t index>
			static void call(_In_ ArgsDialog* _this)
			{
				std::get<index>(_this->m_arg_infos).on_ok();
			};
		};
		struct s_fn_on_cancel
		{
			//通知一行的变量 OnCancel
			template<std::size_t index>
			static void call(_In_ ArgsDialog* _this)
			{
				std::get<index>(_this->m_arg_infos).on_cancel();
			};
		};
		void _OnOK()
		{
			//循环通知每个变量的控件
			iterator_arg_all_index<s_fn_on_ok>();

			EndDialog(IDOK);
		}
		void OnCancel() override
		{
			//屏蔽掉esc键
		}
		void _OnCancel()
		{
			//循环通知每个变量的控件
			iterator_arg_all_index<s_fn_on_cancel>();

			EndDialog(IDCANCEL);
		}
	private:
		bool m_is_checking{};	//由于有时EN_KILLFOCUS会发送两次，使用此变量避免第二次的检查
	private:
		BEGIN_MESSAGE_MAP__IN_CLASS(ArgsDialog)
//				ON_COMMAND_RANGE(m_ui_id_first, m_ui_id_max, &ArgsDialog::OnCtrlCommand)
			ON_BN_CLICKED(m_ui_button_id_ok, &ArgsDialog::_OnOK)
			ON_BN_CLICKED(m_ui_button_id_cancel, &ArgsDialog::_OnCancel)
			ON_WM_SIZE()
		END_MESSAGE_MAP__IN_CLASS()
#pragma endregion
	};
}
