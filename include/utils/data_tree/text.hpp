#pragma once

#include "utils/text/hex.hpp"
#include "utils/text/value.hpp"
#include "utils/data_tree/DataTree.hpp"

//文本格式
namespace utils::data_tree::text
{
	namespace inner
	{
		template<typename T>
		struct s_table_func
		{
			static void from_text(_Out_ std::any & any, _In_ const std::string & type_name, _In_ const std::string & value)
			{
				any = T{};
				utils::text::value::from_text(std::any_cast<T&>(any), value);
			}
			static void to_text(_Out_ std::string & type_name, _Out_ std::string & value, _In_ const std::any & any)
			{
				type_name = typeid(T).name();
				utils::text::value::to_text(value, std::any_cast<const T&>(any));
			}
		};

		struct s_entry
		{
			std::function<void(_Out_ std::any & _value, _In_ const std::string & type_name, _In_ const std::string & value)> from_text;
			std::function<void(_Out_ std::string & type_name, _Out_ std::string & value, _In_ const std::any & any)> to_text;
		};
		using table_t = utils::map<std::string, s_entry>;
		//类型到处理函数的映射
		inline const table_t & table()
		{
			static table_t table = {
		#define entry(type)	{typeid(type).name(), {s_table_func<type>::from_text, s_table_func<type>::to_text}}
				entry(std::string),
				entry(bool),
				entry(char),
				entry(float),
				entry(double),
				entry(int8_t),
				entry(int16_t),
				entry(int32_t),
				entry(int64_t),
				entry(uint8_t),
				entry(uint16_t),
				entry(uint32_t),
				entry(uint64_t),
		#undef entry
			};

			return table;
		}

		namespace simple_type_name
		{
			//变为简化写法
			inline std::string to_simple(_In_ const std::string & type_name)
			{
				if (type_name == typeid(std::string).name())	return "std::string";
				else if (type_name == typeid(int8_t).name())	return "int8_t";
				else if (type_name == typeid(int16_t).name())	return "int16_t";
				else if (type_name == typeid(int32_t).name())	return "int32_t";
				else if (type_name == typeid(int64_t).name())	return "int64_t";
				else if (type_name == typeid(uint8_t).name())	return "uint8_t";
				else if (type_name == typeid(uint16_t).name())	return "uint16_t";
				else if (type_name == typeid(uint32_t).name())	return "uint32_t";
				else if (type_name == typeid(uint64_t).name())	return "uint64_t";
				else
					return type_name;
			}
			//从简化写法变为正常写法
			inline std::string from_simple(_In_ const std::string & type_name)
			{
				if (type_name == "std::string")		return typeid(std::string).name();
				else if (type_name == "int8_t")		return typeid(int8_t).name();
				else if (type_name == "int16_t")	return typeid(int16_t).name();
				else if (type_name == "int32_t")	return typeid(int32_t).name();
				else if (type_name == "int64_t")	return typeid(int64_t).name();
				else if (type_name == "uint8_t")	return typeid(uint8_t).name();
				else if (type_name == "uint16_t")	return typeid(uint16_t).name();
				else if (type_name == "uint32_t")	return typeid(uint32_t).name();
				else if (type_name == "uint64_t")	return typeid(uint64_t).name();
				else
					return type_name;
			}
		}
		
		//从字符串变为值
		inline void from_text(_Out_ std::any & any, _In_ const std::string & type_name, _In_ const std::string & value)
		{
			//从简化类型变回正常表示
			auto _type_name = inner::simple_type_name::from_simple(type_name);

			//已知类型
			if (table().exist(_type_name))
			{
				return table().at(_type_name).from_text(any, _type_name, value);
			}

			//未知类型, 用的十六进制表示

			//转换
			utils::data data;
			auto _value = utils::text::quote::from_quoted(value);
			utils::text::hex::from_text(data, _value, true);

			//给出
			any = utils::serialize::any::inner::BinarySerializeData{_type_name, data};
		}
		//从值变为字符串
		inline void to_text(_Out_ std::string & type_name, _Out_ std::string & value, _In_ const std::any & any)
		{
			//已知类型
			if (table().exist(any.type().name()))
			{
				//转换
				table().at(any.type().name()).to_text(type_name, value, any);

				//简化类型
				type_name = inner::simple_type_name::to_simple(type_name);
				return;
			}

			//准备函数
			auto _to_text = [&type_name, &value](const utils::serialize::any::inner::BinarySerializeData* binary_serialize_data)
			{
				//值: 转为十六进制
				utils::data data;
				utils::text::hex::to_text(value, binary_serialize_data->m_data);
				value = utils::text::quote::to_quoted(value);

				//类型
				type_name = binary_serialize_data->m_type_name;
			};

			//inner::BinarySerializeData类型
			if (any.type() == typeid(utils::serialize::any::inner::BinarySerializeData))
			{
				//转发
				_to_text(std::any_cast<const utils::serialize::any::inner::BinarySerializeData>(&any));
				return;
			}

			//未知类型, 用它的二进制序列化数据表示

			//转换
			utils::data data;
			{
				//除了值的数据，还要加上类型名的二进制数据

				utils::data _data;
				utils::serialize::any::serialize(_data, const_cast<std::any &>(any), true);

				utils::DataWriter output;
				output.serializer & ((std::string)any.type().name());
				output.serializer & _data;

				output.get_data_and_clear(data);
			}

			//转换
			utils::serialize::any::inner::BinarySerializeData binary_serialize_data{any.type().name(), data};

			//转发
			_to_text(&binary_serialize_data);
		}

		//常量
		namespace const_value
		{
			inline const std::string tab = "    ";
			inline const std::string endline = "\n";		//todo 看看这里用不用增加对\r\n 的支持
			inline const std::string version = "DataTreeFormat Version:230512";
			namespace keyword
			{
				inline const std::string array_size = "$array_size";
			}
		}

		struct s_folder
		{
			std::set<std::string> folders;	//文件夹，不带路径
			std::set<std::string> files;	//文件，不带路径
		};

		//获取树上所有的文件夹
		inline void get_folders(_Out_ utils::map<std::string, inner::s_folder> & folders, _In_ utils::DataTree & tree)
		{
			folders.clear();

			//遍历所有文件
			const auto & m_files = tree.help_function.get_m_files();	//todo 想想这里怎么获取比较好
			for (const auto & file : m_files)
			{
				//获取此文件的所有目录和文件名
				std::vector<std::string> names;
				tree.help_function.get_names(names, file.first);

				//俩俩遍历
				std::string _folder;
				for (uint64_t i = 0; i < names.size() - 1; i++)
				{
					_folder += names[i];

					if (i != names.size() - 2)
					{
						//保存子目录名
						folders[_folder].folders.insert(names[i + 1]);
					}
					else
					{
						//保存子文件名
						folders[_folder].files.insert(names[i + 1]);
					}

					_folder += "/";
				}
			}
		}
	}

	//从树变为文本
	inline void to_text(_Out_ std::string & text, _In_ utils::DataTree & tree)	//todo 参数tree变为const类型
	{
		utils::DataWriter output;

		const auto & tab = inner::const_value::tab;
		const auto & endline = inner::const_value::endline;
		namespace keyword = inner::const_value::keyword;

		//版本信息
		output.text.write(inner::const_value::version);
		output.text.write(endline);

		//准备数据
		utils::map<std::string, inner::s_folder> folders;
		inner::get_folders(folders, tree);
		
		//准备函数: 输出名字
		auto write_one_name = [&output](const std::string & name)
		{
			//如果名字中没有空格，则不加引号

			if (((utils::string &)name).string_view().exist(" "))
				output.text.write_quoted_string(name, "\"", "\\");
			else
				output.text.write(name);
		};

		uint64_t tab_n = 0;	//tab的个数
		std::function<void(const std::string & name)> output_folder;
		//准备函数: 输出一个文件夹
		output_folder = [&](const std::string & name)
		{
			//准备函数: 输出一个文件
			//one: 文件名，不包含目录
			auto output_file = [&](_In_ const std::string & one)
			{
				//输出tab
				output.text.write(tab, tab_n);

				//输出名字
				write_one_name(one);

				//输出=
				output.text.write(" = ");

				//准备数据
				std::string value;
				std::string type_name;
				{
					//获取
					auto & any = tree.help_function.get_underlying_representation(name + "/" + one);

					//转换
					inner::to_text(type_name, value, any);
				}

				//输出值
				output.text.write(value);

				//输出tab
				output.text.write(tab);

				//输出类型
				{
					output.write_padding_text(55, 44, 2, ' ');
					output.text.write("//");
					output.text.write(type_name);
					output.text.write(endline);
				}
			};

			//更新状态
			tab_n++;

			//如果是数组
			//note: substr(1)是因为路径以'/'开头，而函数需要的参数不以'/'开头
			if (auto array_size_path = name + "/" + keyword::array_size; false /*todo 实现数组的读取部分，然后去掉这个false*/ && tree.is_file(array_size_path.substr(1)))
			{
				//获取数组大小
				uint64_t array_size{};
				tree.get(array_size_path.substr(1), array_size);

				//简单检查
				utils_assert(folders.get_existed_value(name).files.size() == 1);
				utils_assert(folders.get_existed_value(name).folders.size() == array_size);
				for (auto i : utils::iterator_index(array_size))	//检查文件夹的名字是否是[x]的格式，x是否是从0到array_size-1
				{
					auto _name = name + "/[" + std::to_string(i) + "]";
					utils_assert(tree.is_folder(_name.substr(1)));
				}

				//如果是空数组
				if (array_size == 0)
				{
					//把 数组大小 按普通格式输出

					//输出一行
					output_file(keyword::array_size);
				}
				//否则
				else
				{
					//按数组格式输出

					for (auto i : utils::iterator_index(array_size))
					{
						//输出tab
						output.text.write(tab, tab_n-1);

						//输出:
						output.text.write(':');
						output.text.write(endline);

						//更新状态
						tab_n--;

						//输出内容
						output_folder(name + "/[" + std::to_string(i) + "]");

						//更新状态
						tab_n++;
					}

				}
			}
			//否则
			else
			{
				//输出文件
				for (const auto & one : folders.get_existed_value(name).files)
				{
					//输出一行
					output_file(one);
				}

				//输出文件夹
				for (const auto & one : folders.get_existed_value(name).folders)
				{
					//输出tab
					output.text.write(tab, tab_n);

					//输出名字
					write_one_name(one);

					//输出:
					output.text.write(":");
					output.text.write(endline);

					//输出内容
					output_folder(name + "/" + one);
				}
			}

			//更新状态
			tab_n--;
		};

		//输出顶层文件夹
		output_folder("");

		//输出End
		output.text.write("End");
		output.text.write(endline);

		//给出
		output.get_text(text);
	}
	//从文本变为树
	inline void from_text(_Out_ utils::DataTree & tree, _In_ const std::string & text)
	{
		tree.clear();

		utils::DataReader input{text};

		const auto & tab = inner::const_value::tab;
		const auto & endline = inner::const_value::endline;

		//版本信息
		input.text.read_this(inner::const_value::version);
		input.text.read_this(endline);

		uint64_t tab_n = 1;	//tab的个数
		utils::DataTree::SubTree sub_tree = tree;	//当前的路径前缀

		while (1)
		{
			//读取tab
			{
				uint64_t _tab_n{};
				input.text.read_as_many_times_as_possible(tab, _tab_n);

				//简单检查
				utils_assert(_tab_n <= tab_n);

				//全部读取完毕
				if (_tab_n == 0)	//todo 看看用不用把当前路径变为根节点
					break;

				//返回到相应目录
				if (_tab_n < tab_n)
				{
					//更新状态
					sub_tree = sub_tree.get_parent(tab_n - _tab_n);
					tab_n = _tab_n;
				}
			}

			//读取名字
			std::string name;
			{
				if ('\"' == input.text.peek_one_char())
				{
					input.text.read_quoted_string(name);
				}
				else
				{
					input.text.read_until(name, std::vector<char>{' ', ':'}, true);
				}
			}

			//判断类型
			if (':' == input.text.peek_one_char())
			{
				//是文件夹

				//读取':'
				input.text.read_this(':');
				input.text.read_this(endline);

				//进入下一级
				tab_n++;
				sub_tree = sub_tree.get_sub_tree(name);
			}
			else
			{
				//是文件

				//读取=
				input.text.read_this(" = ");

				//读取值
				std::string value;
				switch(input.text.peek_one_char())
				{
				case '"':
					//读取字符串
					input.text.read_quoted_string(value, false);
					break;
				case '\'':
				{
					//读取字符

					//读取3个字符
					input.text.read(value, 3);
					//如果是转义表示
					if (value[1] == '\\' && value[2] != '\'')
					{
						//再读一个字符
						input.text.read_this('\'');

						value += '\'';
					}
					else
					{
						//简单检查
						utils_assert(value[2] == '\'');
					}
					break;
				}
				default:
					//读取字面量
					input.text.read_until(value, ' ');
				}

				//读取' '
				input.text.read_as_many_times_as_possible(' ');

				//读取"//"
				input.text.read_this("//");

				//读取类型
				std::string type_name;
				input.text.read_until(type_name, endline);

				//生成
				std::any _value;
				inner::from_text(_value, type_name, value);

				//保存
				sub_tree.set(name, _value);
			}
		}

		//读取End
		input.text.read_this("End");
		input.text.read_this(endline);

		//简单检查
		input.readed_all();
	}
}

#if 0
DataTreeFormat Version:230512
    a:
        b:
            1 = 9										 //uint8_t
            2 = 90										 //uint16_t
            3 = 900										 //uint32_t
            4 = 9000									 //uint64_t
            c:                                           
                1 = true								 //bool
                2 = false								 //bool
                3 = true								 //bool
        c:                                               
            1 = -9										 //int8_t
            2 = -90										                     //int16_t
            3asdadsadsdsaadsadfsdfsdfsdfdsdfsfdsfdsfdsfdsfdssdds = -900      //int32_t
            4 = -9000									                     //int64_t
        d:
            1asdadsdasadsadsadsadsasdadsdsadassddassaddasdasdasdssssadss = -9.999912      //float
            2 = -90.999916								                                  //float
            3 = -900.999912349999136                     //double
            4 = -9000.99991234999834				     //double
        e:                                               
            1 = 0									     //char
            2 = ' '									     //char
            3 = ""									     //std::string
            4 = "\\\"\\"							     //std::string
        f:                                               
            1 = -900.999912349999136				     //double
            2 = -9000								     //double
            3 = -900.100000000000023				     //double
            4 = -9000.20000000000073				     //double
        g:                                               
            1 = -0.999912349999123484				     //double
            2 = 0									     //double
            3 = -0.100000000000000006				     //double
            4 = -0.200000000000000011				     //double
End
#endif