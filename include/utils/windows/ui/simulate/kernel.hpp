#pragma once

#include <windows.h>
#include <cfgmgr32.h>
#pragma comment(lib, "Cfgmgr32.lib")
#include <hidsdi.h>
#pragma comment(lib, "Hid.lib")
#include <vector>
#include <string>
#include <utils/Singleton.hpp>
#include <utils/assert.hpp>
#include <utils/map.hpp>
#include <utils/Index2D.hpp>

//模拟鼠标和键盘, 以内核方式
namespace utils::windows::simulate::kernel
{
	namespace inner
	{
		class Device
		{
		private:
			HANDLE m_deviceHandle{};

		public:
			Device(const std::string & interface_prefix)
			{
				GUID guid{};
				HidD_GetHidGuid(&guid);

				ULONG interface_list_length = 0;
				utils_assert(CR_SUCCESS == CM_Get_Device_Interface_List_SizeA(
					&interface_list_length,
					&guid,
					nullptr,
					CM_GET_DEVICE_INTERFACE_LIST_PRESENT
				));
				utils_assert(interface_list_length>1);

				std::vector<char> interface_list;
				interface_list.resize(interface_list_length);
				
				utils_assert(CR_SUCCESS == CM_Get_Device_Interface_ListA(
					&guid,
					nullptr,
					(char*)interface_list.data(),
					interface_list_length,
					CM_GET_DEVICE_INTERFACE_LIST_PRESENT
				));

				std::string lines;
				HANDLE file = INVALID_HANDLE_VALUE;
				for (PSTR currentInterface = &interface_list[0]; *currentInterface; currentInterface += strlen(currentInterface) + 1)
				{
					lines += currentInterface; lines += "\n";
					std::string_view _currentInterface = currentInterface;
					//if (!_currentInterface.starts_with(interface_prefix + "&4784345"))
					if (!_currentInterface.starts_with(interface_prefix))
							continue;

					file = CreateFileA(currentInterface, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
					if (INVALID_HANDLE_VALUE == file)
						continue;

					if (!is_zzvirtual(file))
					{
						CloseHandle(file);
						continue;
					}

					break;
				}

				if (INVALID_HANDLE_VALUE == file) {
					printf("not find the device\n");
					printf("list:\n");
					printf("%s\n", lines.c_str());
					utils_throw();
				}

				m_deviceHandle = file;
			}
			~Device()
			{
				CloseHandle(m_deviceHandle);
			}

		protected:
			void send_report(PVOID data, DWORD size)
			{
				utils_assert(HidD_SetOutputReport(m_deviceHandle, data, size));
			}
		private:
			bool is_zzvirtual(HANDLE file)
			{
				HIDD_ATTRIBUTES attr{};
				if (HidD_GetAttributes(file, &attr))
				{
					/*需要和E:\zz\src\i_say\zz_common\slns\zzvirtual\zzvirtual\public.h中的以下值一致
						VENDOR_ID 
						PRODUCT_ID    
						VERSION_NUMBER    
					*/
					if (
						attr.Size == sizeof(attr) &&
						attr.VendorID == 0x1234 &&
						attr.ProductID == 0x4321 &&
						attr.VersionNumber == 0x12d3
						)
						return true;
				}

				return false;
			}
		};

		class VirtualMouse : public Device
		{
		private:
			enum class Button : uint8_t
			{
				Left = 0x1,
			};
		private:
			uint8_t m_buttons = 0;
		public:
			VirtualMouse()
				: Device{"\\\\?\\HID#HIDCLASS&Col02#1"}
			{
			}
			void click(int x, int y, double scale = 1)
			{
				auto _x = (int)(x/scale);
				auto _y = (int)(y/scale);
				auto re = SetCursorPos(_x,_y);
				//utils_assert(re);
				m_buttons |= (uint8_t)Button::Left;
				send_report();
				m_buttons &= ~(uint8_t)Button::Left;
				send_report();
			}
		private:
			void send_report()
			{
#pragma pack(1)
				struct Report
				{
					BYTE report_id;
					BYTE buttons;
					CHAR x;
					CHAR y;
				};
#pragma pack()
				
				Report report;
				report.report_id            = 0x02;     //鼠标
				report.buttons              = m_buttons;
				report.x                    = 0;
				report.y                    = 0;

				Device::send_report(&report, sizeof(report));
			}
		};
		class VirtualKeyboard : public Device
		{
		public:
			enum class Key : uint8_t
			{
				
//#define KEY_MOD_LCTRL  0x01
//#define KEY_MOD_LSHIFT 0x02
//#define KEY_MOD_LALT   0x04
//#define KEY_MOD_LMETA  0x08
//#define KEY_MOD_RCTRL  0x10
//#define KEY_MOD_RSHIFT 0x20
//#define KEY_MOD_RALT   0x40
//#define KEY_MOD_RMETA  0x80
//
//#define KEY_NONE 0x00 // No key pressed

A = 0x04, // Keyboard a and A
B = 0x05, // Keyboard b and B
C = 0x06, // Keyboard c and C
D = 0x07, // Keyboard d and D
E = 0x08, // Keyboard e and E
F = 0x09, // Keyboard f and F
G = 0x0a, // Keyboard g and G
H = 0x0b, // Keyboard h and H
I = 0x0c, // Keyboard i and I
J = 0x0d, // Keyboard j and J
K = 0x0e, // Keyboard k and K
L = 0x0f, // Keyboard l and L
M = 0x10, // Keyboard m and M
N = 0x11, // Keyboard n and N
O = 0x12, // Keyboard o and O
P = 0x13, // Keyboard p and P
Q = 0x14, // Keyboard q and Q
R = 0x15, // Keyboard r and R
S = 0x16, // Keyboard s and S
T = 0x17, // Keyboard t and T
U = 0x18, // Keyboard u and U
V = 0x19, // Keyboard v and V
W = 0x1a, // Keyboard w and W
X = 0x1b, // Keyboard x and X
Y = 0x1c, // Keyboard y and Y
Z = 0x1d, // Keyboard z and Z

_1 = 0x1e, // Keyboard 1 and !
_2 = 0x1f, // Keyboard 2 and @
_3 = 0x20, // Keyboard 3 and #
_4 = 0x21, // Keyboard 4 and $
_5 = 0x22, // Keyboard 5 and %
_6 = 0x23, // Keyboard 6 and ^
_7 = 0x24, // Keyboard 7 and &
_8 = 0x25, // Keyboard 8 and *
_9 = 0x26, // Keyboard 9 and (
_0 = 0x27, // Keyboard 0 and )

//#define KEY_ENTER 0x28 // Keyboard Return (ENTER)
//#define KEY_ESC 0x29 // Keyboard ESCAPE
//#define KEY_BACKSPACE 0x2a // Keyboard DELETE (Backspace)
TAB = 0x2b, // Keyboard Tab
//#define KEY_SPACE 0x2c // Keyboard Spacebar
//#define KEY_MINUS 0x2d // Keyboard - and _
//#define KEY_EQUAL 0x2e // Keyboard = and +
//#define KEY_LEFTBRACE 0x2f // Keyboard [ and {
//#define KEY_RIGHTBRACE 0x30 // Keyboard ] and }
//#define KEY_BACKSLASH 0x31 // Keyboard \ and |
//#define KEY_HASHTILDE 0x32 // Keyboard Non-US # and ~
//#define KEY_SEMICOLON 0x33 // Keyboard ; and :
//#define KEY_APOSTROPHE 0x34 // Keyboard ' and "
//#define KEY_GRAVE 0x35 // Keyboard ` and ~
//#define KEY_COMMA 0x36 // Keyboard , and <
//#define KEY_DOT 0x37 // Keyboard . and >
//#define KEY_SLASH 0x38 // Keyboard / and ?
//#define KEY_CAPSLOCK 0x39 // Keyboard Caps Lock

F1  = 0x3a, // Keyboard F1
F2  = 0x3b, // Keyboard F2
F3  = 0x3c, // Keyboard F3
F4  = 0x3d, // Keyboard F4
F5  = 0x3e, // Keyboard F5
F6  = 0x3f, // Keyboard F6
F7  = 0x40, // Keyboard F7
F8  = 0x41, // Keyboard F8
F9  = 0x42, // Keyboard F9
F10 = 0x43, // Keyboard F10
F11 = 0x44, // Keyboard F11
F12 = 0x45, // Keyboard F12

//#define KEY_SYSRQ 0x46 // Keyboard Print Screen
//#define KEY_SCROLLLOCK 0x47 // Keyboard Scroll Lock
//#define KEY_PAUSE 0x48 // Keyboard Pause
//#define KEY_INSERT 0x49 // Keyboard Insert
//#define KEY_HOME 0x4a // Keyboard Home
//#define KEY_PAGEUP 0x4b // Keyboard Page Up
//#define KEY_DELETE 0x4c // Keyboard Delete Forward
//#define KEY_END 0x4d // Keyboard End
//#define KEY_PAGEDOWN 0x4e // Keyboard Page Down
//#define KEY_RIGHT 0x4f // Keyboard Right Arrow
//#define KEY_LEFT 0x50 // Keyboard Left Arrow
//#define KEY_DOWN 0x51 // Keyboard Down Arrow
//#define KEY_UP 0x52 // Keyboard Up Arrow
//
//#define KEY_NUMLOCK 0x53 // Keyboard Num Lock and Clear
//#define KEY_KPSLASH 0x54 // Keypad /
//#define KEY_KPASTERISK 0x55 // Keypad *
//#define KEY_KPMINUS 0x56 // Keypad -
//#define KEY_KPPLUS 0x57 // Keypad +
//#define KEY_KPENTER 0x58 // Keypad ENTER
//#define KEY_KP1 0x59 // Keypad 1 and End
//#define KEY_KP2 0x5a // Keypad 2 and Down Arrow
//#define KEY_KP3 0x5b // Keypad 3 and PageDn
//#define KEY_KP4 0x5c // Keypad 4 and Left Arrow
//#define KEY_KP5 0x5d // Keypad 5
//#define KEY_KP6 0x5e // Keypad 6 and Right Arrow
//#define KEY_KP7 0x5f // Keypad 7 and Home
//#define KEY_KP8 0x60 // Keypad 8 and Up Arrow
//#define KEY_KP9 0x61 // Keypad 9 and Page Up
//#define KEY_KP0 0x62 // Keypad 0 and Insert
//#define KEY_KPDOT 0x63 // Keypad . and Delete
//
//#define KEY_102ND 0x64 // Keyboard Non-US \ and |
//#define KEY_COMPOSE 0x65 // Keyboard Application
//
//#define KEY_LEFTCTRL 0xe0 // Keyboard Left Control
//#define KEY_LEFTSHIFT 0xe1 // Keyboard Left Shift
//#define KEY_LEFTALT 0xe2 // Keyboard Left Alt
//#define KEY_LEFTMETA 0xe3 // Keyboard Left GUI
//#define KEY_RIGHTCTRL 0xe4 // Keyboard Right Control
//#define KEY_RIGHTSHIFT 0xe5 // Keyboard Right Shift
//#define KEY_RIGHTALT 0xe6 // Keyboard Right Alt
//#define KEY_RIGHTMETA 0xe7 // Keyboard Right GUI
			};
		private:
			uint8_t m_buttons = 0;
		public:
			VirtualKeyboard()
				: Device{"\\\\?\\HID#HIDCLASS&Col04#1"}
			{
			}
			void type(Key key)
			{
				BYTE key_codes[6]{};

				key_codes[0] = (BYTE)key;
				send_report(key_codes);

				key_codes[0] = 0x00;
				send_report(key_codes);
			}
		private:
			void send_report(BYTE* key_codes)
			{
#pragma pack(1)
				struct Report
				{
					BYTE report_id;
					BYTE modifiers;
					BYTE _reserved;
					BYTE key_codes[6];
				};
#pragma pack()
				Report report{};
				report.report_id            = 0x04;     //键盘
				report.modifiers            = 0x00;
				report._reserved            = 0x00;
				std::memcpy(report.key_codes, key_codes, 6);
				
				Device::send_report(&report, sizeof(report));
			}
		};
	}

	//模拟鼠标点击
	inline void click(int x, int y, double scale = 1)
	{
		return utils::Singleton<utils::windows::simulate::kernel::inner::VirtualMouse>::singleton()->click(x,y,scale);
	}
	//特化
	inline void click(utils::Index2D pos, double scale = 1)
	{
		return click(pos.i, pos.j, scale);
	}

	namespace keyboard
	{
		using Key = utils::windows::simulate::kernel::inner::VirtualKeyboard::Key;

		//模拟键盘输入
		//example:
		//	utils::windows::simulate::kernel::type(utils::windows::simulate::kernel::keyboard::Key::F1);
		inline void type(utils::windows::simulate::kernel::keyboard::Key key)
		{
			return utils::Singleton<utils::windows::simulate::kernel::inner::VirtualKeyboard>::singleton()->type(key);
		}
		//特化
		//example:
		//	utils::windows::simulate::kernel::type("F1");
		inline void type(const std::string & key)
		{
			static const utils::map<std::string,utils::windows::simulate::kernel::keyboard::Key> table{
				{"F1",utils::windows::simulate::kernel::keyboard::Key::F1},
				{"F2",utils::windows::simulate::kernel::keyboard::Key::F2},
				{"F3",utils::windows::simulate::kernel::keyboard::Key::F3},
				{"F4",utils::windows::simulate::kernel::keyboard::Key::F4},
				{"F5",utils::windows::simulate::kernel::keyboard::Key::F5},
				{"F6",utils::windows::simulate::kernel::keyboard::Key::F6},
				{"F7",utils::windows::simulate::kernel::keyboard::Key::F7},
				{"F8",utils::windows::simulate::kernel::keyboard::Key::F8},
				{"F9",utils::windows::simulate::kernel::keyboard::Key::F9},
				{"F10",utils::windows::simulate::kernel::keyboard::Key::F10},
				{"F11",utils::windows::simulate::kernel::keyboard::Key::F11},
				{"F12",utils::windows::simulate::kernel::keyboard::Key::F12},
				{"tab",utils::windows::simulate::kernel::keyboard::Key::TAB},
				{"o",utils::windows::simulate::kernel::keyboard::Key::O},
				{"p",utils::windows::simulate::kernel::keyboard::Key::P},
			};

			return type(table.get_existed_value(key));
		}
	}

	using keyboard::type;
}