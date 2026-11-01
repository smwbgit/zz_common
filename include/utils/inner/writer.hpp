#pragma once

#include <vector>
#include <array>
#include <set>
#include <map>
#include <any>
#include <utils/basic.hpp>
#include <utils/data.hpp>
#include <utils/text/quote.hpp>
#include <utils/text/newline.hpp>
#include <utils/iterator.hpp>
#include <utils/chrono/system_clock.hpp>

#include <utils/compiler/windows_header/maxmin__eliminate.inc>

namespace utils::inner::writer
{
	template<typename Class_T>
	class WithCommonFunctions;
	namespace inner
	{
		template<typename Class_T, uint64_t i, uint64_t size, typename... T>
		struct s_serializer_tuple
		{
			static void write(WithCommonFunctions<Class_T>::s_serializer* _this, const std::tuple<T...>& tuple)
			{
				*_this & std::get<i>(tuple);
			}
		};
		template<typename Class_T, uint64_t size, typename... T>
		struct s_serializer_tuple<Class_T,size,size,T...>
		{
			static void write(WithCommonFunctions<Class_T>::s_serializer* _this, const std::tuple<T...>& tuple)
			{
			}
		};
	}
	//提供一些常用函数
	//note:
	//	上层类需提供以下函数:
	//		void write(_In_ const void* data, _In_ uint64_t size);
	//		void tellp(_Out_ uint64_t & pos);
	template<typename Class_T>
	class WithCommonFunctions : public Class_T
	{
	public:
		using Class_T::Class_T;
		using Class_T::write;

		//写入基本类型或一个结构体
		//note: 结构体需要注意是否需要1字节对齐
		template<typename T, typename std::enable_if<
			utils::is_arithmetic<T>::value || std::is_class<T>::value, int>::type unused = 1>
		void write(const T & value)
		{
//			static_assert(utils::is_arithmetic_v<T> || std::is_class<T>::value, "");
			this->write((char*)&value, sizeof(T));
		}
		//写入一段数据
		void write(_In_ const utils::data & data)
		{
			this->write(data.addr(), data.size());
		}
		//写入一段数据
		void write(_In_ const utils::data_view data)
		{
			this->write(data.addr(), data.size());
		}
		//写入基本类型的数组
		template<typename T>
		void write(const std::vector<T>& data)
		{
			static_assert(utils::is_arithmetic_v<T>, "");

			if (data.empty())
				return;
			this->write(data.data(), data.size() * sizeof(T));
		}
		//写入重复数据
		//value: 数据
		//count: 重复次数
		template<typename T>
		void write_some(const T & value, uint64_t count)
		{
			for (uint64_t i = 0; i < count; i++)
				this->write(value);
		}
#if 0
		//写入一个结构体
		//note: 结构体需要1字节对齐
		template<typename T>
		void write_struct(const T& data)
		{
			static_assert(std::is_class_v<T>, "");
			this->write(&data, sizeof(T));
		}
#endif
		//对齐到指定边界
		//boundary: 边界   e.g.  1 or 2 or 4 or 8
		//fill_char: 为对齐到边界，被写入的字符
		void align(_In_ const uint64_t boundary, _In_ const uint8_t fill_char = 0)
		{
			//当前位置
			uint64_t pos{};
			this->tellp(pos);

			//需要填充的字节数
			uint64_t fill_n = pos % boundary;
			if (fill_n > 0)
				fill_n = boundary - fill_n;

			//填充
			for (auto i : utils::iterator_index(fill_n))
				this->write(fill_char);
		}

#pragma region 大端支持
		struct s_big_endian
		{
		private:
			friend class WithCommonFunctions<Class_T>;
			s_big_endian(WithCommonFunctions<Class_T> & output): m_output{output} {}
			WithCommonFunctions<Class_T> & m_output;
		public:
			template<typename T>
			void write(const T & value)
			{
				//简单检查
				static_assert(utils::is_arithmetic_v<T> || std::is_class<T>::value, "");

				T _value = value;
				utils::basic::change_byte_order(_value);
				m_output.write(_value);
			}
			//写入数组
			template<typename T>
			void write(const std::vector<T>& data)
			{
				//简单检查
				static_assert(utils::is_arithmetic_v<T> || std::is_class<T>::value, "");

				std::vector<T> _data = data;
				utils::basic::change_byte_order(_data);
				m_output.write(_data);
			}
		};
		s_big_endian big_endian{*this};
		s_big_endian big_endian_w{*this};
#pragma endregion
#pragma region 文本处理
		struct s_text
		{
		private:
			friend class WithCommonFunctions<Class_T>;
			s_text(WithCommonFunctions<Class_T> & output) : m_output{output} {}
			WithCommonFunctions<Class_T> & m_output;
		public:
			//写入文本
			//void write(_In_ const std::u8string & text)
			//{
			//	m_output.write(text.c_str(), text.length());
			//}
			//特化
			void write(_In_ std::u8string_view text)
			{
				m_output.write(text.data(), text.length());
			}
			//特化
			void write(_In_ const std::u8string & text, _In_ uint64_t size)
			{
				// 简单检查
				utils_assert(size <= text.length());

				m_output.write(text.data(), size);
			}
			//特化: 写入字符
			void write(_In_ char8_t c)
			{
				m_output.write(&c, 1);
			}
			//写入一个引号引起来的字符串
			//参见: utils::text::quote::to_quoted()
			void write_quoted_string(_In_ const std::string & text, _In_ const std::string & quote, _In_ const std::string & escape)
			{
				this->write(utils::text::quote::to_quoted(text, quote, escape));
			}
			//写入一行
			void write_one_line(_In_ const std::u8string & text, _In_ const std::u8string & newline = utils::text::newline::_n)
			{
				this->write(text);
				this->write(newline);
			}
			//写入多行
			void write_lines(_In_ const std::vector<std::string> & lines, _In_ const std::u8string & newline = utils::text::newline::_n)
			{
				for (const auto & line : lines)
					this->write_one_line(line, newline);
			}
			// 写入多次
			void write_repeated(_In_ const std::u8string & text, _In_ uint64_t count)
			{
				for (auto i : utils::iterator_index(count))
				{
					this->write(text);
				}
			}
		};
		s_text text{*this};
#pragma endregion
#pragma region 序列化支持
		struct s_serializer
		{
#pragma region tuple
		private:
			template<uint64_t i, uint64_t size, typename... T>
			struct s_serializer_tuple
			{
				static void write(WithCommonFunctions<Class_T>::s_serializer* _this, const std::tuple<T...>& tuple)
				{
					*_this & std::get<i>(tuple);
					s_serializer_tuple<i+1, size, T...>::write(_this, tuple);
				}
			};
			template<uint64_t size, typename... T>
			struct s_serializer_tuple<size,size,T...>
			{
				static void write(WithCommonFunctions<Class_T>::s_serializer* _this, const std::tuple<T...>& tuple)
				{
				}
			};
#pragma endregion
		private:
			friend class WithCommonFunctions<Class_T>;
			s_serializer(WithCommonFunctions<Class_T> & output): m_output{output} {}
			WithCommonFunctions<Class_T> & m_output;
		public:
			//获取writer
			WithCommonFunctions<Class_T> & get_writer()
			{
				return m_output;
			}
			//序列化
			template<typename T, typename = std::enable_if_t<utils::is_arithmetic_v<T>>>
			void operator & (const T value)
			{
				m_output.write(value);
			}
			//特化
			void operator & (bool value)
			{
				m_output.write(value ? (uint8_t) 1 : (uint8_t) 0);
			}
			//特化
			void operator & (const std::string & value)
			{
				write_size_t(value.length());
				m_output.write(value.c_str(), value.length());
			}
			//特化
			void operator & (const std::wstring & value)
			{
				write_size_t(value.length());
				m_output.write((char*)value.c_str(), value.length()*sizeof(wchar_t));
			}
			//特化
			void operator & (const std::u8string & value)
			{
				write_size_t(value.length());
				m_output.write((char*)value.c_str(), value.length());
			}
			//特化
			template<typename T>
			void operator & (const std::optional<T> & value)
			{
				*this & value.has_value();
				if(value.has_value())
				{
					*this & value.value();
				}
			}
			//特化
			template<typename T, typename std::enable_if<std::is_class<T>::value, int>::type unused = 1>
			void operator & (const T& value)
			{
				T& _value = const_cast<T&>(value);
				_value.serialize_support<true>(*this);

#if 0			//示例

				//序列化
				template<bool is_saving, typename serializer_T>
				void serialize_support(serializer_T & file)
				{
				}
#endif
			}
			//特化
			template<typename T, std::size_t N>
			void operator & (const std::array<T, N>& arr)
			{
				write_size_t(arr.size());
				for (const auto& e : arr)
					*this & e;
			}
			//特化
			template<typename... T>
			void operator & (const std::tuple<T...>& tuple)
			{
				s_serializer_tuple<0,sizeof...(T),T...>::write(this, tuple);
			}
			//特化
			template<typename T0,typename T1>
			void operator & (const std::map<T0,T1> & map)
			{
				write_size_t(map.size());
				for (const auto& one : map)
				{
					*this & one.first;
					*this & one.second;
				}
			}
			//特化
			template<typename T>
			void operator & (const std::set<T> & container)
			{
				write_size_t(container.size());
				for (const auto & one : container)
				{
					*this & one;
				}
			}
			//特化
			void operator & (const utils::chrono::system_clock::time_point & time_point)
			{
				const auto count = time_point.time_since_epoch().count();

				*this & count;
			}
#if 0	//todo: 让这个能通过编译
			//特化
			void operator & (const std::any & any)
			{
				utils::data data;
				utils::serialize::any::serialize(data, const_cast<std::any &>(any), true);
				*this & data;
			}
#endif
			//特化
			template<typename T, typename std::enable_if<false == utils::is_arithmetic<T>::value, int>::type unused = 1>
			void operator & (const std::vector<T>& vec)
			{
				write_size_t(vec.size());
				for (const auto& e : vec)
					*this & e;
			}
			//特化
			template<typename T, typename std::enable_if<utils::is_arithmetic<T>::value, int>::type unused = 1>
			void operator & (const std::vector<T>& vec)		//todo 这里好像缺个对vector<bool> 的特化
			{
				*this && vec;
			}
			//序列化 快速版
			//note: 快速保存，不循环
			template<typename T>
			void operator && (const std::vector<T>& vec)
			{
				write_size_t(vec.size());
				if (vec.empty() == false)
					m_output.write(&vec[0], vec.size() * sizeof(T));
			}
			//特化
			template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
			void simple_struct(const T & value)
			{
				m_output.write<uint64_t>(sizeof(T));
				m_output.write((uint8_t*)&value, sizeof(T));
			}
		private:
			void write_size_t(std::size_t value)
			{
				m_output.write<uint64_t>(value);
			}
		};
		s_serializer serializer{*this};
#pragma endregion
	};
}

#include <utils/compiler/windows_header/maxmin__restore.inc>
