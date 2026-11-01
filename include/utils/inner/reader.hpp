#pragma once

#include <string>
#include <array>
#include <map>
#include <vector>
#include <any>
#include <utils/text/newline.hpp>
#include <utils/basic.hpp>
#include <utils/u8string.hpp>
#include <utils/operator_overflow_check.hpp>
#include <utils/data.hpp>
#include <utils/text/quote.hpp>
#include <utils/chrono/system_clock.hpp>

namespace utils::serialize::any
{
	//前置声明
	void serialize(utils::data & data, std::any & any, bool is_saving);
}

#include "utils/compiler/windows_header/maxmin__eliminate.inc"

#ifdef utils_check_equal
#error
#endif
#define utils_check_equal(Expect, Actual) \
do { \
	if ((Expect) != (Actual)) \
		utils_throw(); \
}while(0) \


namespace utils::inner::reader
{
	//提供一些常用函数
	//note:
	//	上层类需提供以下函数:
	//		void read(void* data, uint64_t size);
	//		void tellg(uint64_t & pos);
	//		void seekg(uint64_t pos);					//note: 当是tcp流时,要等到指定位置的数据接收到后再返回
	//		void get_pool_size(uint64_t & pool_size);	//note: 当是tcp流时,要等到对方断开连接后再返回
	template<typename Class_T>
	class WithCommonFunctions : public Class_T
	{
	public:
		using Class_T::Class_T;
		using Class_T::read;


		template<typename T>
		T read()
		{
			T value{};
			read<T>(value);
			return value;
		}
		
		//读取一个基本类型或者结构体
		template<typename T>
		void read(T & value)
		{
			static_assert(utils::is_arithmetic_v<T> || std::is_trivially_copyable_v<T>);

			this->read(&value, sizeof(T));
		}

		template<typename T>
		void read(_Out_ std::vector<T> & data, _In_ const uint64_t element_n)
		{
			data.resize(element_n);
			if (data.empty())
				return;
			read(data.data(), data.size() * sizeof(T));
		}
		//读取: 向utils::data里
		void read(_Out_ utils::data & data, _In_ const uint64_t size)
		{
			data.resize(size);
			if (data.empty())
				return;
			read(data.addr(), data.size());
		}
		//读取: 向std::array里
		template<typename T, std::size_t N>
		void read(_Out_ std::array<T, N> & values)
		{
			//简单检查
			static_assert(utils::is_arithmetic_v<T>, "");

			this->read(&values[0], sizeof(T) * N);
		}
		
		//确定接下来的值是这个值，并跳过这个值
		template<typename T>
		void read_this(_In_ const T & expected_value)
		{
			T value{};
			read<T>(value);
			utils_check_equal(value, expected_value);
		}
		//特化
		template<typename T>
		void read_this(const std::vector<T> & expected_values)
		{
			static_assert(utils::is_arithmetic_v<T>, "");

			std::vector<T> values{};
			read<T>(values, expected_values.size());
			utils_check_equal(values, expected_values);
		}
		

		//基于当前位置进行偏移
		//return: 之前的位置
		uint64_t seekg_base_on_cur_pos(_In_ int64_t offset)
		{
			uint64_t old_pos{};
			this->tellg(old_pos);

			//简单检查
			utils_assert(utils::operator_overflow_check::add(old_pos, old_pos, offset));

			this->seekg(old_pos + offset);
			return old_pos;
		}
		//把读取位置，通过向后偏移, 调整到想要对齐的位置
		void seekg_to_align(_In_ uint64_t byte_n)
		{
			if(byte_n == 1) return;

			//简单检查
			utils_assert(byte_n == 2 || byte_n == 4 || byte_n == 8 || byte_n == 16);

			uint64_t old_pos{};
			this->tellg(old_pos);

			uint64_t skip_n{};
			if (old_pos & (byte_n-1))
				skip_n = byte_n - (old_pos & (byte_n-1));

			if(skip_n == 0) return;

			//简单检查
			utils_assert(utils::operator_overflow_check::add(old_pos, old_pos, skip_n));

			this->seekg(old_pos + skip_n);
		}
		//读取一个字段，直到指定的标志
		//example: read_one_field(line, "\n")
		void read_one_field(std::string & field, std::string_view end)
		{
			field.clear();
			while(1)
			{
				char c{};
				read(c);
				field += c;

				if (field.ends_with(end))
				{
					field.erase(field.length() - end.length());
					return;
				}
			}
		}
		void read_this_field(std::string_view expected_field, std::string_view end)
		{
			std::string field;
			read_one_field(field, end);
			utils_check_equal(field, expected_field);
		}
		//读取一行
		void read_one_line(std::string & line, char endl = '\n')
		{
			line.clear();
			while(1)
			{
				char c{};
				read(c);
				if (c == endl)
					return;

				line += c;
			}
		}
		void read_this_line(const std::string & expected_line, char endl = '\n')
		{
			std::string line;
			read_one_line(line, endl);
			utils_check_equal(line, expected_line);
		}

		void read_uninterested_bytes(uint64_t size)
		{
			//算出新位置
			uint64_t pos{};
			this->tellg(pos);
			uint64_t new_pos = pos + size;

			//转到新位置
			this->seekg(new_pos);
		}
		//是否已读完全部文件
		bool is_readed_all()
		{
			uint64_t pool_size{};
			this->get_pool_size(pool_size);
			uint64_t pos{};
			this->tellg(pos);

			//简单检查
			utils_assert(pos <= pool_size);

			return pos == pool_size;
		}
		//确定已读完全部文件
		void readed_all()
		{
			utils_assert(is_readed_all());
		}
#if 0
		//读取所有数据
		void read_whole_pool(_Out_ std::vector<uint8_t> & content)
		{
			uint64_t pool_size{};
			this->get_pool_size(pool_size);
			this->seekg(0);
			this->read(content, pool_size);
		}
		//特化
		void read_whole_pool(_Out_ utils::data & content)	//todo 看看怎么统一众多的参数类型的变形
		{
			uint64_t pool_size{};
			this->get_pool_size(pool_size);
			this->seekg(0);
			this->read(content, pool_size);
		}
		//特化
		void read_whole_pool(_Out_ std::string & content)
		{
			uint64_t pool_size{};
			this->get_pool_size(pool_size);
			this->seekg(0);
			content.resize(pool_size);
			this->read((void*)content.c_str(), content.length());
		}
#endif
		//读取所有数据
		void read_all(_Out_ std::vector<uint8_t> & data)
		{
			uint64_t size{};
			this->get_pool_size(size);
			this->seekg(0);
			this->read(data, size);
		}
		//特化
		void read_all(_Out_ utils::data & data)
		{
			uint64_t size{};
			this->get_pool_size(size);
			this->seekg(0);
			this->read(data, size);
		}
		//特化
		void read_all(_Out_ std::string & text)
		{
			uint64_t size{};
			this->get_pool_size(size);
			this->seekg(0);
			text.resize(size);
			this->read((void*)text.c_str(), text.length());
		}
		//特化
		template<typename T = utils::data>
		T read_all()
		{
			T data{};
			read_all(data);
			return data;
		}
		//获取剩余部分的大小
		uint64_t get_rest_size()
		{
			uint64_t pool_size{};
			this->get_pool_size(pool_size);
			uint64_t cur_pos{};
			this->tellg(cur_pos);
			return pool_size - cur_pos;
		}
		//读取剩余部分
		void read_rest(_Out_ std::vector<uint8_t> & rest)
		{
			uint64_t rest_size = get_rest_size();
			read(rest, rest_size);
		}
		//特化
		void read_rest(_Out_ utils::data & rest)
		{
			uint64_t rest_size = get_rest_size();
			read(rest, rest_size);
		}
		//读取指定位置指定大小的数据块
		void read_part_of_pool(_Out_ std::vector<uint8_t> & content, _In_ uint64_t pos, _In_ uint64_t size)
		{
			this->seekg(pos);
			this->read(content, size);
		}
		//特化
		void read_part_of_pool(_Out_ utils::data & content, _In_ uint64_t pos, _In_ uint64_t size)
		{
			//转发
			read_part_of_pool(content.m_data, pos, size);
		}
		//偷看一个值
		//note: 不改变读取位置
		template<typename... T>
		void peek(_Out_ T && ... value)
		{
			//todo 看看能不能和try_peek()版本合在一起写

			//记录当前位置
			uint64_t pos{};
			this->tellg(pos);

			//读取
			this->read(std::forward<T>(value)...);

			//恢复当前位置
			this->seekg(pos);
		}
		//特化
		template<typename... T>
		bool try_peek(_Out_ T && ... value)
		{
			//记录当前位置
			uint64_t pos{};
			this->tellg(pos);

			//读取
			try
			{
				this->read(std::forward<T>(value)...);
			}
			catch (utils::Exception & e)
			{
				//恢复当前位置
				this->seekg(pos);
				return false;
			}

			//恢复当前位置
			this->seekg(pos);
			return true;
		}
		
		
#pragma region 大端支持
		struct s_big_endian
		{
		private:
			friend class WithCommonFunctions<Class_T>;
			s_big_endian(WithCommonFunctions<Class_T> & input): m_input{input} {}
			WithCommonFunctions<Class_T> & m_input;
		public:
			template<typename T>
			void read(T & value)
			{
				static_assert(utils::is_arithmetic_v<T> || std::is_class<T>::value);

				m_input.read(value);
				utils::basic::change_byte_order(value);
			}
			//特化
			template<typename T>
			T read()
			{
				T value{};
				read<T>(value);
				return value;
			}
			template<typename T>
			void read(std::vector<T> & data, uint64_t size_in_elements)
			{
				m_input.read(data, size_in_elements);
				utils::basic::change_byte_order(data);
			}
			template<typename T>
			void read_this(T expected_value)
			{
				static_assert(utils::is_arithmetic_v<T>, "");

				T value{};
				read<T>(value);
				utils_check_equal(value, expected_value);
			}
		};
		s_big_endian big_endian{*this};
		s_big_endian big_endian_r{*this};
#pragma endregion

#pragma region 文本处理
		template<typename Char_T>
		struct s_text
		{
		private:
			friend class WithCommonFunctions<Class_T>;
			WithCommonFunctions<Class_T> & m_input;
			s_text(WithCommonFunctions<Class_T> & input) : m_input{input} {}

			using basic_string = std::basic_string<Char_T>;
			using basic_string_view = std::basic_string_view<Char_T>;
		public:
			//读取
			//length: 文本长度
			void read(_Out_ basic_string & text, _In_ const uint64_t length)
			{
				text.resize(length);
				m_input.read((void*)text.data(), text.length()*sizeof(Char_T));
			}
			//特化：读取一个以0结尾的字符串，把内容放入text，抛弃结尾0
			void read(_Out_ basic_string & text)
			{
				this->read_until(text, (Char_T)0);
			}
			//特化：读取一个字符
			void read(_Out_ Char_T & c)
			{
				m_input.read(c);
			}
			//读取全部内容
			basic_string read_all()
			{
				uint64_t size{};
				m_input.get_pool_size(size);

				//简单检查
				utils_assert(size % sizeof(Char_T) == 0);

				m_input.seekg(0);
				basic_string text;
				this->read(text, size / sizeof(Char_T));
				return text;
			}
			//确定接下来的值是这个文本(不包括结尾0)，并跳过这个文本
			void read_this(_In_ basic_string_view expected_text)
			{
				basic_string text;
				this->read(text, expected_text.length());
				utils_check_equal(text, expected_text);
			}
			//特化: 重复读取
			void read_this(_In_ basic_string_view expected_text, _In_ uint64_t count)
			{
				for (auto i : utils::iterator_index(count))
				{
					this->read_this(expected_text);
				}
			}
			//特化: char类型
			void read_this(_In_ Char_T expected_char)
			{
				Char_T c{};
				m_input.read(c);
				utils_check_equal(c, expected_char);
			}
			//读取指定字符串尽可能多次
			//text: 要读取的字符串
			//readed_n: 读到多少个
			void read_as_many_times_as_possible(_In_ basic_string_view text, _Out_ uint64_t & readed_n)
			{
				readed_n = 0;
				while (1)
				{
					//偷看下一个字符串
					std::string next;
					if (false == utils_try(peek(next, text.length())))
					{
						//读取完毕
						return;
					}

					//如果相同
					if (next == text)
					{
						//读取
						read_this(text);
						
						//更新状态
						readed_n++;
						continue;
					}

					//读取完毕
					return;
				}
			}
			//特化
			void read_as_many_times_as_possible(_In_ basic_string_view text)
			{
				uint64_t readed_n{};

				//转发
				return read_as_many_times_as_possible(text, readed_n);
			}
			//特化
			void read_as_many_times_as_possible(_In_ Char_T c)	//todo 看看怎么把char传入std::string_view中，这样就不用写这个特化函数了
			{
				Char_T text[2] = {c, 0};

				//转发
				return read_as_many_times_as_possible(text);
			}
			//读取，直到遇到指定字符串中的一个
			//note: mark不会放入text中
			//is_keep_mark: 是否恢复mark到输入流中，也就是，mark接下来是否会被读取到
			void read_until(_Out_ basic_string & text, _In_ const std::vector<basic_string> & marks, _In_ bool is_keep_mark = false)
			{
				text.clear();

				auto is_meet_mark = [&text, &marks, is_keep_mark, this]()
				{
					for (const auto & mark : marks)
					{
						if (basic_string_view(text).ends_with(mark))
						{
							//去掉text里的mark
							basic_string_view view = text;
							view.remove_suffix(mark.length());
							text = view;

							//恢复input里的mark
							if (is_keep_mark)
								m_input.seekg_base_on_cur_pos(- (int64_t)mark.length());

							return true;
						}
					}
					return false;
				};

				while (1)
				{
					//读取
					Char_T c{};
					this->read(c);
					text += c;

					//读完
					if (is_meet_mark())
						return;
				}
			}
			//特化
			void read_until(_Out_ basic_string & text, _In_ const basic_string & mark, _In_ bool is_keep_mark = false)
			{
				return read_until(text, std::vector<basic_string>{mark}, is_keep_mark);
			}
			//特化
			void read_until(_Out_ basic_string & text, _In_ const Char_T mark, _In_ bool is_keep_mark = false)
			{
				return read_until(text, basic_string{} + mark, is_keep_mark);
			}
			//特化
			void read_until(_Out_ basic_string & text, _In_ const std::vector<Char_T> & marks, _In_ bool is_keep_mark = false)
			{
				std::vector<basic_string> _marks;
				for (const auto mark : marks)
					_marks.push_back(basic_string{} + mark);

				return read_until(text, _marks, is_keep_mark);
			}
			//偷看
			//length: 文本长度
			void peek(_Out_ basic_string & text, _In_ const uint64_t length)
			{
				text.resize(length);
				m_input.peek((void*)text.data(), text.length()*sizeof(Char_T));
			}
			//偷看一个字符
			Char_T peek_one_char()
			{
				Char_T c{};
				m_input.peek(c);
				return c;
			}
			//读取剩余所有内容
			void read_rest(_Out_ basic_string & rest)
			{
				uint64_t rest_size = m_input.get_rest_size();

				//简单检查
				utils_assert(rest_size % sizeof(Char_T) == 0);

				this->read(rest, rest_size / sizeof(Char_T));
			}
#if 0	//默认参数中的字符串不知道怎么写
			//读取所有行
			template<typename T>
			void read_all_lines(_Out_ T & lines, _In_ const std::u8string & newline = utils::text::newline::_n)
			{
				std::u8string text = this->read_all();
				lines = utils::u8string(text).split<T>(newline);
			}
			//读取剩余行
			void read_rest_lines(_Out_ std::vector<std::u8string> & lines, _In_ const std::u8string & newline = utils::text::newline::_n)
			{
				std::u8string rest;
				this->read_rest(rest);
				lines = utils::u8string(rest).split(newline);
			}
			//读取一个引号引起来的字符串
			//参见: utils::text::quote::from_quoted()
			//is_remove_quote: 返回前，是否去掉引号
			void read_quoted_string(_Out_ basic_string & text, _In_ bool is_remove_quote = true, _In_ const basic_string & quote = "\"", _In_ const std::string & escape = "\\")
			{
				text.clear();

				//读取开始的quote
				this->read_this(quote);
				text += quote;

				//准备函数: 判断最后一个quote是否被转义了
				auto is_last_quote_escaped = [&quote, &escape](_In_ const std::string & text)
				{
					std::string_view view = text;

					//移除开始的quote
					view.remove_prefix(quote.length());

					//是否在转义
					bool is_escaping = false;

					while (1)
					{
						//简单检查
						utils_assert(view.empty() == false);	//因为text的最后是quote

						if (view.starts_with(quote))
						{
							if (view.size() == 1)
							{
								//到达最后
								return is_escaping;
							}
							else
							{
								//简单检查
								utils_assert(is_escaping);	//中间的引号需要是转义的

								//更新状态
								is_escaping = false;

								view.remove_prefix(1);
							}
						}
						else if (view.starts_with(escape))
						{
							//更新状态
							is_escaping = !is_escaping;

							view.remove_prefix(escape.length());
						}
						else
						{
							//更新状态
							is_escaping = false;

							view.remove_prefix(1);
						}
					}
				};

				while (1)
				{
					//读取一个字符
					{
						char c{};
						m_input.read(c);
						text += c;
					}

					//如果读取到了quote
					if (std::string_view(text).ends_with(quote))
					{
						//如果quote没被转义
						if (false == is_last_quote_escaped(text))
						{
							//读取完毕
							break;
						}
					}
				}

				//去掉引号
				if (is_remove_quote)
					text = utils::text::quote::from_quoted(text, quote, escape);
			}
#endif
#if 0		//index可能会跳过, 重新实现一下
			//读取一行，不关心读取的内容
			void read_one_uninterested_line(const std::u8string & newline = utils::text::newline::_n)
			{
				//读取，直到读完finish_flag或者读到eof
				//note: 不能直接读到eof
				auto read_until_readed_this_or_eof = [this](const std::u8string & finish_flag)
				{
					bool is_first_read = true;
					uint64_t index{};
					while (1)
					{
						if (index == finish_flag.length()) return;

						char c{};
						if (utils_fail(this->read(c)))
						{
							if (is_first_read)
								utils_throw();
							return;
						}
						is_first_read = false;

						if (c == (char)finish_flag[index])
						{
							index++;
							continue;
						}

						index = 0;
					}
				};
				read_until_readed_this_or_eof(newline);
			}
			//读取一些行，不关心读取的内容
			//line_n: 行数
			void read_some_uninterested_lines(uint64_t line_n, const std::u8string & new_line_character = utils::text::newline::_n)
			{
				for (uint64_t i = 0; i < line_n; i++)
					read_one_uninterested_line(new_line_character);
			}
#endif
		};
		s_text<char> text{*this};
		s_text<char8_t> text_u8{*this};
		s_text<char16_t> text_u16{*this};
#pragma endregion
#pragma region 序列化支持
		struct s_serializer
		{
#pragma region tuple
		private:
			template<uint64_t i, uint64_t size, typename ...T>
			struct s_serializer_tuple
			{
				static void read(_In_ WithCommonFunctions<Class_T>::s_serializer* _this, _Out_ std::tuple<T...> & tuple)
				{
					*_this & std::get<i>(tuple);
					s_serializer_tuple<i+1, size, T...>::read(_this, tuple);
				}
			};
			template<uint64_t size, typename ...T>
			struct s_serializer_tuple<size,size,T...>
			{
				static void read(_In_ WithCommonFunctions<Class_T>::s_serializer* _this, _Out_ std::tuple<T...> & tuple)
				{
				}
			};
#pragma endregion
		private:
			friend class WithCommonFunctions<Class_T>;
			s_serializer(WithCommonFunctions<Class_T> & input): m_input{input} {}
			WithCommonFunctions<Class_T> & m_input;
		public:
			//反序列化
			template<typename T, typename = std::enable_if_t<utils::is_arithmetic<T>::value, void>>
			void operator & (T & value)
			{
				m_input.read(value);
			}
			//特化
			void operator & (bool & value)
			{
				uint8_t c{};
				m_input.read(c);
				utils_assert(c == 0 || c == 1);
				value = c != (uint8_t) 0;
			}
			//特化
			void operator & (std::string & value)
			{
				std::size_t length{};
				read_size_t(length);

				value.resize(length);
				m_input.read((char*)value.c_str(), length);
			}
			//特化
			void operator & (std::wstring & value)
			{
				std::size_t length{};
				read_size_t(length);

				value.resize(length);
				m_input.read((char*)value.c_str(), length*sizeof(wchar_t));
			}
			//特化
			void operator & (std::u8string & value)
			{
				std::size_t length{};
				read_size_t(length);

				value.resize(length);
				m_input.read((char*)value.c_str(), length);
			}
			//特化
			template<typename T>
			void operator & (std::optional<T> & value)
			{
				value.reset();

				bool has_value{};
				*this & has_value;

				if(has_value)
				{
					T v{};
					*this & v;
					value = v;
				}
			}
			//特化
			template<typename T, typename std::enable_if<std::is_class<T>::value, int>::type unused = 1>
			void operator & (T & value)
			{
				value.serialize_support<false>(*this);

#if 0			//示例
				
				//序列化
				template<bool is_saving, typename serializer_T>
				void serialize_support(serializer_T & file)
				{
				}
#endif
			}
			//特化
			template <typename T, std::size_t N>
			void operator & (std::array<T, N> & arr)
			{
				std::size_t size{};
				read_size_t(size);
				utils_assert(N == size);

				for (auto & e : arr)
					*this & e;
			}
			//特化
			template <typename ...T>
			void operator & (std::tuple<T...> & tuple)
			{
				s_serializer_tuple<0, sizeof...(T), T...>::read(this, tuple);
			}
			//特化
			template <typename T0,typename T1>
			void operator & (std::map<T0,T1> & map)
			{
				std::size_t size{};
				read_size_t(size);

				for (auto i : utils::iterator_index(size))
				{
					T0 key{};
					T1 value{};
					*this & key;
					*this & value;

					map[std::move(key)] = std::move(value);
				}
			}
			//特化
			template <typename T>
			void operator & (std::set<T> & container)
			{
				std::size_t size{};
				read_size_t(size);

				for (auto i : utils::iterator_index(size))
				{
					T value{};
					*this & value;

					container.insert(std::move(value));
				}
			}
			//特化
			void operator & (utils::chrono::system_clock::time_point & time_point)
			{
				decltype(time_point.time_since_epoch().count()) count{};

				*this & count;

				time_point = utils::chrono::system_clock::time_point{
					utils::chrono::system_clock::duration{count}
				};
			}
			//特化
			void operator & (std::any & any)
			{
				utils::data data;
				*this & data;
				utils::serialize::any::serialize(data, any, false);
			}
			//特化
			template <typename T, typename std::enable_if<false == utils::is_arithmetic<T>::value, int>::type unused = 1>
			void operator & (std::vector<T> & vec)
			{
				std::size_t size{};
				read_size_t(size);

				vec.resize(size);
				for (auto & e : vec)
					*this & e;
			}
			//特化
			template <typename T, typename std::enable_if<utils::is_arithmetic<T>::value, int>::type unused = 1>
			void operator & (std::vector<T> & vec)
			{
				*this && vec;
			}
			//特化
			void operator & (std::vector<bool> & vec)
			{
				std::size_t size{};
				read_size_t(size);

				vec.resize(size);
				for (auto && e : vec)
				{
					bool value{};
					*this & value;
					e = value;
				}
			}
			//反序列化  快速版
			template <typename T>
			void operator && (std::vector<T> & vec)
			{
				std::size_t size{};
				read_size_t(size);

				vec.resize(size);
				if (vec.empty() == false)
					m_input.read(&vec[0], size * sizeof(T));
			}
			//特化
			template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
			void simple_struct(T & value)
			{
				uint64_t size{};
				m_input.read<uint64_t>(size); utils_assert(size == sizeof(T));

				m_input.read((uint8_t*)&value, sizeof(T));
			}
		private:
			void read_size_t(std::size_t & value)
			{
				uint64_t tmp{};
				m_input.read(tmp);
				utils::basic::store_in(value, tmp);
			}
		};
		s_serializer serializer{*this};
#pragma endregion
	};
}


#undef utils_check_equal

#include "utils/compiler/windows_header/maxmin__restore.inc"
