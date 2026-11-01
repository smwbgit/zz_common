#pragma once

#include <string>
#include <stdarg.h>
#include <regex>
#include <utils/assert.hpp>
#include <utils/type_traits.hpp>
#include <utils/text/encoding.hpp>
#include <utils/text/value.hpp>
#include <utils/basic/store_this_integer.hpp>
#include <utils/iterator.hpp>
#include <utils/u8string_view.hpp>
#include <utils/integer_type.hpp>
#include <utils/text/split.hpp>

namespace utils
{
	//向前声明
	namespace inner
	{
		class u8string;
	}

	//向前声明
	inline const inner::u8string & u8string(const std::u8string & s);
	inline inner::u8string & u8string(std::u8string & s);

	namespace inner
	{
		class u8string : public std::u8string
		{
		public:
			using std::u8string::u8string;
			using std::u8string::operator =;

			//替换
		public:
			//全部替换
			//return: 替换了多少处
			uint64_t replace_all(const std::u8string & from, const std::u8string & to)
			{
				//from如果为空，就直接返回
				if (from.empty()) return 0;

				std::u8string result;
				std::size_t begin_pos = 0;
				uint64_t count{};
				while (1)
				{
					std::size_t pos = this->find(from, begin_pos);
					if (pos == this->npos)
					{
						result += this->substr(begin_pos);
						break;
					}

					result += this->substr(begin_pos, pos - begin_pos);
					result += to;
					begin_pos = pos + from.length();
					count++;
				}

				*this = std::move(result);
				return count;
			}
#if 0	//todo 修改好，这段代码
			//查找所有符合的字符串,将其替换
			//regex: 要被替换的字符串
			//meet_one_word: 根据找到的字符串, 返回替换后的字符串
			void replace_all(const std::regex & regex, std::function<std::u8string(const std::u8string & word)> meet_one_word)
			{
				std::u8string result;
				{
					std::u8string str = std::move(*this);
					std::smatch sm;
					while(std::regex_search((const std::string &)str, sm, regex))
					{
						result += (std::u8string &&)(std::string)sm.prefix();
						result += meet_one_word((std::u8string &&)sm.str());
						str = (std::u8string &&)(std::string)sm.suffix();
					}
					result += str;
				}
				*this = std::move(result);
			}
#endif
			//替换为windows的换行符
			void replace_to_windows_newline()
			{
				replace_all(u8"\n", u8"\r\n");
			}

			//移除
		public:
			//删掉所有指定的字符串，返回删掉次数
			uint64_t remove_all(_In_ const std::u8string & not_want)
			{
				//简单检查
				utils_assert(not_want.empty() == false);

				std::u8string result;
				size_t pos{};
				uint64_t count{};
				while (1)
				{
					auto pos2 = this->find(not_want, pos);
					if (pos2 == this->npos) break;

					result += this->substr(pos, pos2 - pos);

					pos = pos2 + not_want.length(); count++;
				}

				result += this->substr(pos);
				*this = std::move(result);
				return count;
			}
			//删掉所有被括起来的内容
			//note: 如果有一个begin,那么其后边必须有个end. 其中的内容会被删除掉, 包括begin和end
			//is_include_marks: 删除时是否包含begin和end这两个
			//e.g.:
			//	"you look great.(by the way. balabala.)bye" -> "you look great.bye"			with args: "("  ")"
			void remove_all_quoted(_In_ const std::u8string & begin, _In_ const std::u8string & end, bool is_include_marks)
			{
				//简单检查
				utils_assert(begin.empty() == false);
				utils_assert(end.empty() == false);

				std::u8string result;
				size_t pos{};
				while (1)
				{
					auto pos2 = this->find(begin, pos);
					if (pos2 == this->npos) break;

					result += this->substr(pos, pos2 - pos + (is_include_marks ? 0 : begin.length()));

					auto pos3 = this->find(end, pos2 + begin.length());

					//简单检查
					utils_assert(pos3 != this->npos);

					pos = pos3 + (is_include_marks ? end.length() : 0);
				}

				result += this->substr(pos);
				*this = std::move(result);
			}
			//消除两边的空白字符
			//not_want: 空白字符
			void remove_trim(_In_ char8_t not_want = ' ')
			{
				auto pos = this->find_first_not_of(not_want);
				auto pos2 = this->find_last_not_of(not_want);

				if (pos == this->npos)
				{
					this->clear();
					return;
				}
				*this = this->substr(pos, pos2 - pos + 1);
			}
			//移除两边的单引号
			void remove_a_pair_of_single_quotes()
			{
				//简单检查
				utils_assert(this->length() >= 2);
				utils_assert((*this)[0] == '\'');
				utils_assert((*this)[this->length() - 1] == '\'');

				*this = this->substr(1, this->length() - 2);
			}

			//格式化
		public:
			//格式化
			u8string & format(const char8_t* format, ...)
			{
				va_list args;
				va_start(args, format);
				format_vargs(format, args);
				va_end(args);
				return *this;
			}
			//特化
			u8string & format_vargs(const char8_t* format, va_list va)
			{
				*this = utils::text::format::format_vargs(format, va);
				return *this;
			}

#if 0
			//比较
		public:
			//是否相同: 不区分大小写
			bool compare_is_same(const std::u8string & o, bool is_ignore_case = false) const
			{
				if (is_ignore_case)
				{
					return std::equal(this->begin(), this->end(), o.begin(), o.end(), [](char8_t a, char8_t b)
					{
						return toupper(a) == toupper(b);
					});
				}
				else
					return *this == o;
			}
#endif
			//
		public:
			auto view() const
			{
				return utils::u8string_view((std::u8string_view)*this);
			}

			//编码转换
		public:
			std::u16string encoding_to_u16() const
			{
				return utils::text::encoding::utf8_to_utf16(*this);
			}
			u8string & encoding_from_u16(const std::u16string & u16)
			{
				*this = utils::text::encoding::utf16_to_utf8(u16);
				return *this;
			}

			//类型转换
		public:
			std::string cast_to_string() const
			{
				return (const std::string &)*this;
			}
			void cast_from_string(const std::string & text)
			{
				*this = (const std::u8string &)text;
			}
		
			//序列化
		public:
			template<bool is_saving, typename T>
			void serialize_support(T& file)
			{
				auto & t = (std::string &) (*this);
				file & t;
			}

			//数字和文本间转换
		public:
			//转换字符串到数字
			template<typename T, typename = std::enable_if_t<utils::is_arithmetic_v<T>>>
			T number_out() const
			{
				T value{};
				utils::text::value::from_text(value, *this);
				return value;
			}
			//特化
			template<typename T, typename = std::enable_if_t<utils::is_integral_v<T>>>
			T number_out(uint64_t base = 10) const
			{
				T value{};
				utils::text::value::from_text(value, *this, base);
				return value;
			}

	#if 0
			//转换字符串到数字
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, float>>>
			T number_to() const
			{
				try { T value = std::stof(m_text); return value; }
				catch(std::exception &) { utils_throw(); }
			}
			//特化
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, double>>>
			T number_to() const
			{
				try { T value = std::stod(m_text); return value; }
				catch(std::exception &) { utils_throw(); }
			}
			//特化
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, uint64_t>>>
			T number_to(uint64_t base = 10) const
			{
				size_t pos{};
				T value{};
				try { value = std::stoull(m_text, &pos, (int)base); }
				catch(std::exception &) { utils_throw(); }

				//简单检查
				utils_assert(pos == m_text.length());

				return value;
			}
			//特化
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, uint32_t>>>
			T number_to(uint64_t base = 10) const
			{
				auto tmp = number_to<uint64_t>(base);
				return utils::basic::store_in<T>(tmp);
			}
			//特化
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, int>>>
			T number_to(uint64_t base = 10) const
			{
				size_t pos{};
				T value{};
				try { value = std::stoi(m_text, &pos, (int)base); }
				catch(std::exception &) { utils_throw(); }

				//简单检查
				utils_assert(pos == m_text.length());

				return value;
			}
			//特化
			template<typename T, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>>
			T number_to(uint64_t base = 10) const
			{
				auto tmp = number_to<int>(base);
				return utils::basic::store_in<T>(tmp);
			}
	#endif

			//赋值
		public:
			//把字节数组转为字符串
			void assign_byte_array(_In_ const std::vector<uint8_t> & vec)
			{
				this->assign(vec.begin(), vec.end());
			}
			//把字符串转为字节数组, 不包括结尾的\0
			void assign_byte_array_out(_Out_ std::vector<uint8_t> & vec) const
			{
				vec.assign(this->begin(), this->begin() + this->length());
			}
			//把一段内存转为字符串
			//is_truncate_by_0: 遇到0时是否截断
			void assign_memory(_In_ const void* data, _In_ uint64_t size, _In_ bool is_truncate_by_0 = false)
			{
				uint64_t _size = size;
				if (is_truncate_by_0)
				{
					_size = strnlen((const char*)data, size);
				}
				this->assign((const char8_t*)data, _size);
			}
		
			//拼接
		public:
			//把字符串数组进行拼接
			//vec_text: 字符串数组
			//separator: 分隔符
			//is_ended_by_spliter: 是否是以分隔符结尾.如果不是,则以字符串数组的最后一个结尾
			void concatenate(_In_ const std::vector<std::u8string> & vec_text, _In_ const std::u8string & separator, _In_ bool is_ended_by_spliter = false)
			{
				this->clear();

				for (const auto & one : utils::iterator_and_give_me_i(vec_text))
				{
					*this += one.value;
					if (false == is_ended_by_spliter && one.i == vec_text.size() - 1)
						;
					else
						*this += separator;
				}
			}
		
			//查找
		public:
			//是否存在指定字符串
			bool find_is_exist(const std::u8string & text) const
			{
				auto pos = this->find(text);
				return pos != this->npos;
			}
			//特化
			bool is_exist(const std::u8string & text) const
			{
				return find_is_exist(text);
			}

			//大小写
		public:
			//首字母大写，其他字母小写
			//例如：
			//  "ABC" -> "Abc"
			//  "abc" -> "Abc"
			//  "1BC" -> "1bc"
			//  "1bc" -> "1bc"
			void case_capitalize()
			{
				case_to_lower();
				if (!this->empty())
					(*this)[0] = toupper((uint8_t)(*this)[0]);
			}
			//转为小写
			void case_to_lower()
			{
				std::transform(this->begin(), this->end(), this->begin(),
					[](char8_t c) -> char8_t { return tolower((uint8_t)c); });
			}
			//转为大写
			void case_to_upper()
			{
				std::transform(this->begin(), this->end(), this->begin(),
					[](char8_t c) -> char8_t { return toupper((uint8_t)c); });
			}
		
			//分割
		public:
			//分割字符串
			//separator: 分割符
			//is_keep_empty: 是否保留空字符串
			std::vector<std::u8string> split(
				_In_ const std::u8string & separator,
				_In_ bool is_keep_empty = true) const
			{
				std::vector<std::u8string> fields;
				{
					utils::text::split::RealImpl<> split{*this};
					split.set_separator(separator);
					split.m_is_keep_empty = is_keep_empty;
					split.set_result_array(fields);
					split.execute();
				}
				return fields;
			}
		
			//二进制
		public:
			//把不方便显示的字符转为十六进制显示, 以便于把字符串完整显示出来
			std::u8string binary_get_printable_string() const
			{
				std::u8string text;
				bool is_first = true;
				bool is_showing_char = false;
				for (const auto c : *this)
				{
					if (isprint((uint8_t)c))
					{
						if (is_showing_char == false)
						{
							if (is_first)
							{
								is_first = false;
							}
							else
							{
								text += u8", ";
							}
							text += u8"'";
							is_showing_char = true;
						}

						text += c;
					}
					else
					{
						if (is_showing_char)
						{
							text += u8"'";
							is_showing_char = false;
						}

						if (is_first)
						{
							is_first = false;
						}
						else
						{
							text += u8", ";
						}
						text += u8string().format(u8"0x%02x", (uint8_t)c);
					}
				}
				if (is_showing_char)
				{
					text += u8"'";
					is_showing_char = false;
				}
				if (false == text.empty())
					text = u8"[[printable_string(" + text + u8")]]";
				return text;
			}

			//提取
		public:
			//把字符串依次对应一个值
			void substr_solve(_Out_ std::u8string & out_0, _In_ const std::u8string & in_0, _Out_ std::u8string & out_1) const
			{
				auto pos = this->find(in_0);
			
				//简单检查
				utils_assert(pos != in_0.npos);
			
				//提取
				out_0 = this->substr(0, pos);
				out_1 = this->substr(pos + in_0.length());
			}
			//用指定的字符串来截取
			//note:
			//	当有多个begin匹配时,取第一个
			//	当begin后有多个end时,取第一个end
			//	当end为空时, 截取到字符串结束
			//	当指定的begin或end不存在时, 报错
			//e.g.:
			//	s="123_a_456_b_789_b_000_a_111_b_222";
			//	s.substr_by_string(s2, "_a_", "_b_");
			//	此时 s2 == "456"
			//e.g.:
			//	s="123_a_456_b_789";
			//	s.substr_by_string(s2, "_a_");
			//	此时 s2 == "456_b_789"
			//e.g.:
			//	s="123456789";
			//	s.substr_by_string(s2, "_a_");
			//	此时 抛出异常
			//e.g.:
			//	s="123_a_456789";
			//	s.substr_by_string(s2, "_a_", "_b_");
			//	此时 抛出异常
			std::u8string substr_by_string(_In_ const std::u8string & begin, _In_ const std::u8string & end = {}) const
			{
				//简单检查
				utils_assert(false == begin.empty());

				auto pos = this->find(begin);
				utils_assert(pos != this->npos);
				if (end.empty())
				{
					return this->substr(pos + begin.length());
				}
				else
				{
					auto pos2 = this->find(end, pos + begin.length());
					utils_assert(pos2 != this->npos);
				
					auto pos1 = pos + begin.length();
					return this->substr(pos1, pos2 - pos1);
				}
			}
			//截取第n个分隔符后边的全部字符
			//note: nth不可以为0
			//args:
			//	split_string: 分隔符
			//	nth: 第几个
			//e.g.:
			//	s = "1,2,3,4,5"
			//	s.substr_by_nth_string(s2, ",", 2)
			//	此时 s2 == "3,4,5"
			std::u8string substr_by_nth_string(_In_ const std::u8string & split_string, _In_ const uint64_t nth) const
			{
				//简单检查
				utils_assert(nth != 0);
				utils_assert(false == split_string.empty());

				size_t pos{};
				for (uint64_t i = 0; i < nth; i++)
				{
					pos = this->find(split_string, pos);
				
					//简单检查
					utils_assert(pos != this->npos);

					pos += split_string.length();
				}

				return this->substr(pos);
			}
			//is_include_marks: 结果中是否包含begin和end这两个mark
			template<typename T_vector>
			T_vector substr_by_two_mark(const std::u8string & begin, const std::u8string & end, bool is_include_marks)
			{
				T_vector vector;
				uint64_t i = 0;
				while(1)
				{
					auto _begin = this->find(begin, i);
					if(_begin == this->npos) break;
					
					auto _end = this->find(end, i+begin.length());
					if(_end == this->npos) break;

					vector.push_back(is_include_marks ?
						this->substr(_begin, _end-_begin+end.size()):
						this->substr(_begin+begin.size(), _end-_begin-begin.size()));

					i=_end+end.length();
				}
				return vector;
			}

			//重复
		public:
			//重复
			std::u8string repeat(u8 times) const
			{
				std::u8string text;
				for(auto i : utils::iterator_index(times))
					text += *this;
				return text;
			}
		};
	}
	inline const inner::u8string & u8string(const std::u8string & s)
	{
		return (const inner::u8string &)s;
	}
	inline inner::u8string & u8string(std::u8string & s)
	{
		return (inner::u8string &)s;
	}
}

#if 0 //string_view

		//starts_with() 特化: 如果存在此前缀，则去掉此前缀
		bool compare_starts_with(const string & prefix, bool is_remove)
		{
			bool re = starts_with(prefix);

			//去掉前缀
			if (is_remove && re)
				remove_prefix(prefix.length());

			return re;
		}
#endif
#if 0	//半成品
		
		//利用回调函数来确定分隔符
		void split_by_custom_rule(
			_In_ std::function<void(_In_ const std::u8string & word)> meet_one_word,
			_In_ bool is_keep_empty = true
		)
		{
			fragments.clear();

			std::string word;
			for (const auto c : *this)
			{
				if (is_this_a_spliter(c))
				{
					if (word.empty()== false)
					{
						fragments.push_back(word);
						word.clear();
					}
				}
				else
				{
					word += c;
				}
			}

			if (word.empty()== false)
			{
				fragments.push_back(word);
				word.clear();
			}
		}
#endif
#if 0
namespace utils::string_function::不知该放哪儿
{
	//生成字符串数组，vec[0] = "prefix_0"，vec[1] = "prefix_1", ...
	//prefix: 前缀
	//n: 个数
	inline void make_string_vector(_Out_ std::vector<std::string> & vec, const std::string & prefix, uint64_t n)
	{
		vec.resize(n);
		for (auto i : utils::iterator_index(n))
			vec[i] = prefix + std::to_string(i);
	}

	//向一个字符数组里保存一个字符串。如果字符串过长，出错。如果没有指定c，字符数组应该和text一样长
	//char_array: 字符数组
	//text: 字符串
	//c: 当字符串比较短时的填充字符
	//示例1:
	//	char arr[4]{};
	//	store_this_string(arr, "abcd");
	//	arr == "abcd";
	//示例2:
	//	char arr[4]{};
	//	store_this_string(arr, "abc", ' ');
	//	arr == "abc ";
	template<std::size_t N>
	void store_this_string(_Out_ char (&char_array)[N], _In_ const std::string & text, _In_ char c)
	{
		utils_assert(N >= text.length());								//简单检查
		memmove(char_array, text.c_str(), text.length());				//复制
		std::fill(char_array + text.length(), char_array + N, c);		//填充
	}
	//特化
	template<std::size_t N>
	void store_this_string(_Out_ char (&char_array)[N], _In_ const std::string & text)
	{
		utils_assert(N == text.length());								//简单检查
		memmove(char_array, text.c_str(), text.length());				//复制
	}

	//格式化数字
	namespace format_number
	{
		//获得类型在printf/scanf里的占位符
		//e.g.	get_..<uint64_t>(false)  ->  %llu
		//		get_..<uint64_t>(true)   ->  %llx
		//------------
		//特化: 整数
		template<typename T, typename unused = std::enable_if_t<utils::is_integral_v<T>>>
		std::string get_placeholder_of_printf(_In_ const bool is_hex, _In_ const bool is_hex_uppercase = true)
		{
			auto size =
				sizeof(T) == 1 ? "hh" :
				(sizeof(T) == 2 ? "h" :
				(sizeof(T) == 4 ? "" :
				(/*sizeof(T) == 8 ? */"ll")));
			auto sign = is_hex ? (is_hex_uppercase ? "X" : "x") : (std::is_signed<T>::value ? "d" : "u");
			return std::string() + "%" + size + sign;	//todo 变为编译期常量
		}
		//特化: 浮点数
		template<typename T, typename unused = std::enable_if_t<std::is_floating_point_v<T>>>
		std::string get_placeholder_of_printf()
		{
			//todo 变为编译期常量

			if constexpr(std::is_same_v<T, float>)
			{
				return "%f";
			}
			else if constexpr(std::is_same_v<T, double>)
			{
				return "%lf";
			}
			else
			{
				static_assert(utils::language_support::always_false<T>);
			}
		}

		//试着转为整数
		//is_hex: 字符串里是否为十六进制
		template<typename T>
		bool try_to_integer(_Out_ T & value, _In_ const std::string & text, bool is_hex = false)
		{
			//简单检查
			static_assert(utils::is_integral_v<T>, "");

			//按有符号的读
			using signed_T = typename std::make_signed<T>::type;
			char c{};	//多读一个字节，如果读出来了，说明有多余字符，返回false
			auto fmt = utils::string_function::format_number::get_placeholder_of_printf<signed_T>(is_hex) + "%c";
			signed_T signed_value{};
			if (1 != sscanf(text.c_str(), fmt.c_str(), &signed_value, &c))
				return false;

			//无符号时检查是否为负
			if (std::is_signed<T>::value == false)
			{
				if (signed_value < 0)
					return false;
			}

			//传出
			value = (T)signed_value;
			return true;
		}
	}
}
#endif