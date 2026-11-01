#pragma once

#include <functional>
#include <thread>
#include <string>
#include <windows.h>
#include <mutex>
#include <utils/assert.hpp>
#include <utils/map.hpp>
#include <utils/windows/ui/message.hpp>

namespace utils::windows::ui
{
	//增加一些全局热键
	//note: 线程安全
	//note: 当类析构时，所有热键会自动注销
	//example: 
	//	utils::windows::ui::HotKeyManager manager;
	//	manager.add_hot_key("alt", 'B', []()
	//	{
	//		//do what you want do
	//	});
	class HotKeyManager
	{
	private:
		struct s_entry
		{
			uint32_t id{};
			std::function<void()> callback;
			std::string modifier_str;
			UINT modifier;
			char key;
		};
	private:
		std::atomic<uint32_t> m_next_hotkey_id{0};
		std::unique_ptr<std::thread> m_thread;
		std::map<uint32_t /*id*/, s_entry> m_hotkeys;
		std::mutex m_lock;	//for m_thread and m_hotkeys
		static constexpr UINT UM_ADD_HOTKEY = WM_USER+1;
		static constexpr UINT UM_REMOVE_HOTKEY = WM_USER+2;
		static constexpr UINT UM_REMOVE_ALL_HOTKEY = WM_USER+3;
		std::atomic<bool> m_is_thread_ready{};
	public:
		HotKeyManager()
		{
			//启动线程
			start_thread();
		}
		~HotKeyManager()
		{
			//通知注销全部
			utils::windows::ui::message::post_message_to_thread(*m_thread, UM_REMOVE_ALL_HOTKEY, 0, 0);

			//等待
			m_thread->join();
		}
	public:
		//增加一个全局热键
		//modifier: "alt" or "shift" or "ctrl"
		//key:  'A' 到 'Z'
		//meet_hot_key__call_from_other_thread:   当热键按下时的回调函数. 注意:此函数会从其他线程中调用
		void add_hot_key(
			_In_ const std::string & modifier, _In_ const char key,
			_In_ std::function<void()> meet_hot_key__call_from_other_thread)
		{
			//简单检查
			utils_assert(key >= 'A' && key <= 'Z');

			//准备modifiers
			UINT _modifiers{};
			if (modifier == "alt")			_modifiers = MOD_ALT;
			else if (modifier == "ctrl")	_modifiers = MOD_CONTROL;
			else if (modifier == "shift")	_modifiers = MOD_SHIFT;
			else							utils_throw();
			_modifiers |= MOD_NOREPEAT;

			//准备id
			const auto hotkey_id = m_next_hotkey_id.fetch_add(1);

			//保存回调函数
			{
				//加锁
				std::lock_guard<std::mutex> guard{m_lock};

				m_hotkeys[hotkey_id] = {
					hotkey_id,
					meet_hot_key__call_from_other_thread,
					modifier,
					_modifiers,
					key
				};
			}

			//在线程里注册热键
			utils::windows::ui::message::post_message_to_thread(*m_thread, UM_ADD_HOTKEY, hotkey_id, 0);
		}

		//删除一个全局热键
		//modifier: "alt" or "shift" or "ctrl"
		//key:  'A' 到 'Z'
		void remove_hot_key(_In_ const std::string & modifier, _In_ const char key)
		{
			uint32_t hotkey_id{};
			{
				//加锁
				std::lock_guard<std::mutex> guard{m_lock};

				//查找条目
				{
					bool is_find{};
					for (const auto & e : m_hotkeys)
					{
						if (e.second.modifier_str != modifier || e.second.key != key)
							continue;

						hotkey_id = e.first;
						is_find = true;
						break;
					}
					utils_assert(is_find);
				}
			}

			//通知注销
			utils::windows::ui::message::post_message_to_thread(*m_thread, UM_REMOVE_HOTKEY, hotkey_id, 0);
		}
	private:
		//启动线程
		void start_thread()
		{
			m_thread.reset(new std::thread{[&]()
			{
				//创建消息队列
				utils::windows::ui::message::create_message_queue();

				//更新状态
				m_is_thread_ready = true;

				MSG msg{};
				while (GetMessage(&msg, NULL, 0, 0))
				{
					switch (msg.message)
					{
					case WM_HOTKEY:
					{
						//查找回调函数
						std::function<void()> _meet_hot_key;
						{
							//加锁
							std::lock_guard<std::mutex> guard{m_lock};

							const auto hotkey_id = (uint32_t)msg.wParam;
							_meet_hot_key = utils::map(m_hotkeys).get_existed_value(hotkey_id).callback;
						}

						//调用
						_meet_hot_key();
						break;
					}
					case UM_ADD_HOTKEY:
					{
						//准备数据
						uint32_t hotkey_id = (uint32_t)msg.wParam;
						UINT modifiers{};
						UINT key{};
						{
							//加锁
							std::lock_guard<std::mutex> guard{m_lock};

							const auto & e = utils::map(m_hotkeys).get_existed_value(hotkey_id);
							modifiers = e.modifier;
							key = (UINT)e.key;
						}

						//注册
						utils_assert(RegisterHotKey(NULL, hotkey_id, modifiers, key));
						break;
					}
					case UM_REMOVE_HOTKEY:
					{
						//准备数据
						uint32_t hotkey_id = (uint32_t)msg.wParam;
				
						//注销
						utils_assert(UnregisterHotKey(NULL, hotkey_id));
						
						{
							//加锁
							std::lock_guard<std::mutex> guard{m_lock};

							//删除条目
							m_hotkeys.erase(hotkey_id);
						}
						break;
					}
					case UM_REMOVE_ALL_HOTKEY:
					{
						//加锁
						std::lock_guard<std::mutex> guard{m_lock};
						
						//注销
						for (const auto & e : m_hotkeys)
						{
							const auto hotkey_id = e.first; 
							utils_assert(UnregisterHotKey(NULL, hotkey_id));
						}

						//删除条目
						m_hotkeys.clear();

						//退出线程
						PostQuitMessage(0);
						break;
					}
					}
				}
			}});

			//等待线程创建出消息队列
			while (m_is_thread_ready == false)
				std::this_thread::yield();
		}
	};
}
