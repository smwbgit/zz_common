#pragma once

#include <mutex>
#include "utils/vector.hpp"
#include "utils/string.hpp"
#include "utils/data_tree/DataTree.hpp"
#include "libs/libmodbus/import.hpp"

namespace utils
{
	//线程
	//note: 在stop之后，可以再次start
	class Thread
	{
	public:
		struct Information
		{
			//线程是否被要求停止
			std::atomic_bool is_need_stop{};
		};

	private:
		std::unique_ptr<std::thread> m_thread;
		Information m_information;

	public:
		//开始
		void start(std::function<void(utils::Thread::Information & thread_information)> thread_func)
		{
			//简单检查
			utils_assert(m_thread.get() == nullptr);

			//准备数据
			m_information.is_need_stop = false;

			//创建线程
			m_thread.reset(new std::thread{[this, thread_func]()
			{
				thread_func(m_information);
			}});
		}
		//停止
		void stop()
		{
			//简单检查
			utils_assert(m_thread);

			//设置停止信号
			set_stop_flag();

			//等待结束
			join();
		}
		//设置停止信号
		void set_stop_flag()
		{
			//简单检查
			utils_assert(m_thread);

			//设置标志
			m_information.is_need_stop = true;
		}
		//等待结束
		void join()
		{
			//简单检查
			utils_assert(m_thread);

			//等待结束
			m_thread->join();

			//删除
			m_thread.reset(nullptr);
		}
	public:
		//帮助函数
		class helper
		{
		public:
			//停止一些线程
			template<typename... Ts>
			static void stop(Ts && ... threads)
			{
				set_stop_flag(std::forward<Ts>(threads)...);
				join(std::forward<Ts>(threads)...);
			}
		private:
			//设置停止信号: 一些线程
			template<typename... Ts>
			static void set_stop_flag(Ts && ... threads)
			{
				_set_stop_flag(std::forward<Ts>(threads)...);
			}
			//实现
			template<typename T, typename... Ts>
			static void _set_stop_flag(T && threads, Ts && ... other)
			{
				_set_stop_flag(threads);
				_set_stop_flag(other);
			}
			//特化
			static void _set_stop_flag(std::vector<Thread> & threads)
			{
				for (auto & thread : threads)
					_set_stop_flag(thread);
			}
			//特化
			static void _set_stop_flag(Thread & thread)
			{
				thread.set_stop_flag();
			}
			//等待结束: 一些线程
			template<typename... Ts>
			static void join(Ts && ... threads)
			{
				_join(std::forward<Ts>(threads)...);
			}
			//实现
			template<typename T, typename... Ts>
			static void _join(T && threads, Ts && ... other)
			{
				_join(threads);
				_join(other);
			}
			//特化
			static void _join(std::vector<Thread> & threads)
			{
				for (auto & thread : threads)
					_join(thread);
			}
			//特化
			static void _join(Thread & thread)
			{
				thread.join();
			}
		};
	};
	//多个线程
	class Threads
	{
	public:
		class Information
		{
		public:
			const uint64_t index{};
		};

	private:
		std::vector<Thread> m_threads;

	public:
		//当前线程个数
		uint64_t size()
		{
			return m_threads.size();
		}
		//添加一些线程
		//n: 添加的个数
		void add(uint64_t n, std::function<void(utils::Thread::Information & thread_information, utils::Threads::Information & threads_information)> thread_func)
		{
			//起始索引
			uint64_t begin_index = size();

			//添加一些线程
			for (auto i : utils::iterator_index(n))
			{
				//创建并开始
				Thread thread;
				thread.start([this, thread_func, index = i + begin_index](utils::Thread::Information & thread_information)
				{
					utils::Threads::Information threads_information{index};
					thread_func(thread_information, threads_information);
				});
				
				//保存
				m_threads.push_back(std::move(thread));
			}
		}
	};
}

namespace utils::network::modbus::tcp
{
	//把多个ip上的modbus服务器合并为一个，并在当前ip上提供一个新的合并后的modbus服务器
	class IpCombiner
	{
	private:
		//块
		struct s_block
		{
			uint16_t addr{};	//起始地址
			uint16_t size{};	//大小
			utils::vector<uint16_t> data;		//数据
			utils::vector<uint16_t> data_new;	//数据
		};
		//子服务器。对应于一个要被合并的modbus服务器
		struct s_child_server
		{
			utils::string ip;
			uint16_t port{};
			utils::vector<s_block> blocks;
			std::unique_ptr<std::atomic<std::chrono::steady_clock::time_point>> last_update_time;	//上次写入时间	//atomic不允许复制，就加了层指针
		};

	private:
		uint64_t m_timeout_ms{};	//超时时间, 单位毫秒
		uint16_t m_alarm_addr{};	//报警地址
		uint16_t m_alarm_value{};	//报警数值
		std::vector<s_child_server> m_child_servers;	//所有的子服务器
		uint16_t m_server_port{};	//服务器端口
		uint16_t m_server_registers_n{};	//服务器的寄存器个数
		std::atomic<bool> m_is_alarm = false;	//此程序本身是否报警
		modbus_mapping_t* m_server_mapping{};//服务器的内存
		std::mutex m_lock;	//同步
		utils::Threads m_client_threads;	//客户端线程
		utils::Thread m_server_thread;	//服务器线程
		utils::Thread m_checker_thread;	//超时检查线程

#pragma warning()
//		sdf sdfjj;	//todo 检查所有变量，从stop再到start时
	public:
		//设置参数
		void set_options(utils::DataTree::SubTree tree)
		{
#if 0                   
DataTreeFormat Version:230512
    ip_combiner:
        timeout_ms = 9000                 //uint64_t
        alarm_addr = 90                   //uint16_t
        alarm_value = 90                  //uint16_t
        server_port = 90                  //uint16_t
        server_registers_n = 90           //uint16_t
        child_servers:
        :
            ip = ""                       //std::string
            port = ""                     //uint16_t
            blocks:
            :
                addr = ""                 //uint16_t
                port = ""                 //uint16_t
            :
                addr = ""                 //uint16_t
                port = ""                 //uint16_t
                    
#elif 0
DataTreeFormat Version:230512
    ip_combiner:
        timeout_ms = 5000                 //uint64_t
        alarm_addr = 1                   //uint16_t
        alarm_value = 2                  //uint16_t
        server_port = 502                  //uint16_t
        server_registers_n = 3000           //uint16_t
        child_servers:
            $array_size = 1               //uint64_t
            [0]:
                ip = "127.0.0.1"                       //std::string
                port = 10000                     //uint16_t
                blocks:
                    $array_size = 2               //uint64_t
                    [0]:
                        addr = 10000                 //uint16_t
                        size = 2                 //uint16_t
                    [1]:
                        addr = 10003                 //uint16_t
                        size = 1                 //uint16_t
End

#endif
			tree.get("timeout_ms", m_timeout_ms);	//超时时间
			tree.get("alarm_addr", m_alarm_addr);	//报警地址
			tree.get("alarm_value", m_alarm_value);	//报警数值
			tree.get("server_port", m_server_port);	//服务器端口
			tree.get("server_registers_n", m_server_registers_n);	//服务器的寄存器个数

			//所有的子服务器
			m_child_servers.clear();
			tree.get_array("child_servers", [&](uint64_t index, utils::DataTree::SubTree tree)
			{
				s_child_server c;

				tree.get("ip", c.ip.wrapped_type());
				tree.get("port", c.port);
				c.last_update_time.reset(new std::atomic<std::chrono::steady_clock::time_point>{std::chrono::steady_clock::now()});
				tree.get_array("blocks", [&](uint64_t index, utils::DataTree::SubTree tree)
				{
					s_block b;

					tree.get("addr", b.addr);
					tree.get("size", b.size);

					c.blocks.push_back(b);
				});

				m_child_servers.push_back(std::move(c));
			});
		}
		//开始运行：在另一个线程上
		void start()
		{
			//创建服务器线程
			create_server();

			//创建客户端线程
			create_clients();

			//创建超时检查线程
			create_checker();
		}
		//停止运行
		void stop()
		{
			//最后停服务器，防止其他线程用到服务器的寄存器

			//等待结束
			utils::Thread::helper::stop(m_client_threads, m_checker_thread);
			utils::Thread::helper::stop(m_server_thread);
		}
	private:
		//创建客户端线程
		void create_clients()
		{
			//简单检查
			utils_assert(m_client_threads.size() == 0);

			m_client_threads.add(m_child_servers.size(), [this](utils::Thread::Information & thread_information, utils::Threads::Information & threads_information)
			{
				client_thread(thread_information, threads_information);
			});
		}
		void client_thread(utils::Thread::Information & thread_information, utils::Threads::Information & threads_information)
		{
			const uint64_t child_server_index = threads_information.index;
			modbus_t* ctx{};

			while (1)
			{
				//是否退出
				if (thread_information.is_need_stop)
					break;

				//让出cpu
				std::this_thread::yield();

//				printf("connecting: %s:%d\n", g_machines[machine_index].ip.c_str(), g_machines[machine_index].port);

				//连接modbus服务器
				ctx = modbus_new_tcp(m_child_servers[child_server_index].ip.c_str(), m_child_servers[child_server_index].port);
				if (modbus_connect(ctx) == -1)
				{
					modbus_free(ctx);
					continue;
				}

//				printf("connect success: %s:%d\n", g_machines[machine_index].ip.c_str(), g_machines[machine_index].port);

				while (1)
				{
					//是否退出
					if (thread_information.is_need_stop)
						break;

					//让出cpu
					std::this_thread::yield();

					//读取每个块
					if (false == read_all_blocks(ctx, child_server_index))
						break;

					//是否和之前的一样
					if (false == is_same_as_last(child_server_index))
						continue;

					//放入服务器的数据区
					copy_data_to_server(child_server_index);

					//置当前时间
					*(m_child_servers[child_server_index].last_update_time) = std::chrono::steady_clock::now();
				}

				//关闭连接
				modbus_close(ctx);
				modbus_free(ctx);

				//using namespace std::chrono_literals;
				//std::this_thread::sleep_for(1s);
			}
		}
		//读取每个块
		bool read_all_blocks(modbus_t* ctx, uint64_t child_server_index)
		{
			for (s_block & b : m_child_servers[child_server_index].blocks)
			{
				b.data_new.resize(b.size);

				//读取
				if (b.size != modbus_read_registers(ctx, b.addr, b.size, &b.data_new[0]))
					return false;
			}
			return true;
		}
		//是否和之前的一样
		bool is_same_as_last(uint64_t child_server_index)
		{
			bool is_same = true;

			for (s_block & b : m_child_servers[child_server_index].blocks)
			{
				if (b.data != b.data_new)
				{
					is_same = false;
				}

				//数据横移
				b.data = b.data_new;
			}
			return is_same;
		}
		//放入服务器的数据区
		void copy_data_to_server(uint64_t child_server_index)
		{
			std::lock_guard<std::mutex> lock_guard{m_lock};

			for (s_block & b : m_child_servers[child_server_index].blocks)
			{
				memmove(m_server_mapping->tab_registers + b.addr, &b.data[0], b.size * 2);
			}

			//放入报警信息
			if (m_is_alarm)
			{
				*(m_server_mapping->tab_registers + m_alarm_addr) = m_alarm_value;
			}
		}
		//创建检查超时线程
		void create_checker()
		{
			m_checker_thread.start([this](utils::Thread::Information & thread_information)
			{
				checker_thread(thread_information);
			});
		}
		//检查每个线程是否超时
		void checker_thread(utils::Thread::Information & thread_information)
		{
			m_is_alarm = false;
			int i = 0;

			while (1)
			{
				//是否退出
				if (thread_information.is_need_stop)
					break;

				//让出cpu
				std::this_thread::yield();

				bool is_alarm = false;
				for (auto & child_server : m_child_servers)
				{
					if (std::chrono::steady_clock::now() - child_server.last_update_time.get()->load() >= std::chrono::milliseconds{m_timeout_ms})
					{
						is_alarm = true;
					}
				}
				m_is_alarm = is_alarm;

//				if (is_alarm)
//				{
//					printf("alarm\n");
//					Sleep(1000);
//				}
			}
		}
		//创建服务器线程
		void create_server()
		{
			m_server_thread.start([&](utils::Thread::Information & thread_information)
			{
				server_thread(thread_information);
			});
		}
		void server_thread(utils::Thread::Information & thread_information)
		{
			//简单检查
			utils_assert(m_server_mapping == nullptr);

			//分配内存
			m_server_mapping = modbus_mapping_new(0, 0, m_server_registers_n, 0);

			//简单检查
			utils_assert(m_server_mapping);

			//建立监听
			modbus_t* ctx = modbus_new_tcp(0, m_server_port);
			utils_assert(ctx);
//			if (ctx == 0)
//				throw "fail: modbus_new_tcp()";		//todo  看看哪个是端口占用时，会报错的函数. 把另一个改为utils_throw. 把这个改为throw error(error_id);
			int s = modbus_tcp_listen(ctx, 5);
			utils_assert(s != -1);
//			if (s == -1)
//				throw "fail: modbus_tcp_listen";

//			printf("server started\n");

			//准备数据
			std::atomic<uint64_t> thread_count{};	//统计连入的客户端线程，还有多少在运行

			//等待连入
			while (1)
			{
				//是否退出
				if (thread_information.is_need_stop)
					break;

				//让出cpu
				std::this_thread::yield();

				//等待连入
				int client_sock = modbus_tcp_accept(ctx, &s);	//todo 把这个改为非阻塞的
				if (client_sock < 0)
					continue;

				//创建线程来应答
				thread_count++;
				std::thread{[this, &thread_information, client_sock, &thread_count]()
				{
					client_thread_for_server(thread_information, client_sock);

					thread_count--;
				}}.detach();
			}

			//设置退出标志，等待所有应答线程退出
			thread_information.is_need_stop = true;
			while (thread_count > 0)
				std::this_thread::yield();

			//关闭监听
			modbus_close(ctx);
			modbus_free(ctx);

			//释放内存
			modbus_mapping_free(m_server_mapping);
			m_server_mapping = nullptr;
		}
		void client_thread_for_server(utils::Thread::Information & thread_information, int client_sock)
		{
			modbus_t* ctx{};

			utils_assert(ctx = modbus_new_tcp(0, m_server_port));	//todo 看看这里获取ctx的方式是否可以
			utils_assert(-1 != modbus_set_socket(ctx, client_sock));

			while (1)
			{
				uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH]{};
				int rc{};
				int reply_result{};

				rc = modbus_receive(ctx, query);
				if (rc == 0)
					continue;
				if (rc == -1)
					break;

				{
					std::lock_guard<std::mutex> lock_guard{m_lock};

					reply_result = modbus_reply(ctx, query, rc, m_server_mapping);	//todo 看看这里可不可以把发送放到锁定外边
				}

				if (-1 == reply_result)
					break;
			}
			modbus_close(ctx);
			modbus_free(ctx);
		}
	};
}
