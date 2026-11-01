#pragma once

#include <string>
#include <any>
#include <functional>
#include <vector>
#include "utils/map.hpp"
#include "utils/DataReader.hpp"
#include "utils/DataWriter.hpp"
#include "utils/CallWhenProgramStart.hpp"

//std::any
namespace utils::serialize::any
{
	namespace inner
	{
		struct s_one_type
		{
			std::function<void(utils::DataWriter & output, std::any & any)> save;
			std::function<void(utils::DataReader & input, std::any & any)> load;
		};

		inline utils::map<std::string, s_one_type> map_type_to_serializer;

		template<typename T>
		class s_serializer
		{
		public:
			static void save(utils::DataWriter & output, std::any & any)
			{
				output.serializer & std::any_cast<T&>(any);
			}
			static void load(utils::DataReader & input, std::any & any)
			{
				any = T{};
				input.serializer & std::any_cast<T&>(any);
			}
		};

		//用二进制序列化数据表示的std::any
		class BinarySerializeData
		{
		public:
			std::string m_type_name;
			utils::data m_data;
		public:
			template<bool is_saving, typename T>
			void serialize(T& file)
			{
				if constexpr (is_saving)
				{
					//需要清除已经写入的类型名。因为写入的类型名是BinarySerializeData，而不是原来的类型

					file.get_writer().clear();
					file.get_writer().write(m_data);
				}
				else
				{
					utils_throw();
				}
			}
			void load_unregister_type(const std::string & type_name, utils::DataReader & input)
			{
				m_type_name = type_name;
				input.read_whole_pool(m_data);
			}
		};
	}

	//对std::any进行序列化和反序列化
	inline void serialize(utils::data & data, std::any & any, bool is_saving)
	{
		if (is_saving)
		{
			utils::DataWriter output;

			//类型名
			std::string type_name = any.type().name();

			//保存类型名
			output.serializer & type_name;

			//保存其中的对象
			{
				//获得对应类型的序列化函数
				auto & save = inner::map_type_to_serializer.get_existed_value(type_name).save;

				save(output, any);
			}

			//给出
			output.get_data(data);
		}
		else
		{
			utils::DataReader input{data};

			//读取类型名
			std::string type_name;
			input.serializer & type_name;

			//读取其中的对象
			{
				//如果注册了对应类型的序列化函数
				if (inner::map_type_to_serializer.exist(type_name))
				{
					//交给它来载入
					auto & load = inner::map_type_to_serializer.get_existed_value(type_name).load;
					load(input, any);
				}
				else
				{
					//载入为一个BinarySerializeData
					any = inner::BinarySerializeData{};
					std::any_cast<inner::BinarySerializeData&>(any).load_unregister_type(type_name, input);
				}
			}

			//简单检查
			input.readed_all();
		}
	}

	//添加一个可序列化的类型
	template<typename T>
	void add_serializer()
	{
		//简单检查
		utils_assert(false == inner::map_type_to_serializer.exist(typeid(T).name()));

		inner::map_type_to_serializer[(std::string)typeid(T).name()] = {
			inner::s_serializer<T>::save,
			inner::s_serializer<T>::load,
		};
	}

	namespace inner
	{
		//添加常用类型的序列化函数
		CallWhenProgramStart([]()
		{
			utils::serialize::any::add_serializer<bool>();
			utils::serialize::any::add_serializer<char>();
			utils::serialize::any::add_serializer<float>();
			utils::serialize::any::add_serializer<double>();
			utils::serialize::any::add_serializer<int8_t>();
			utils::serialize::any::add_serializer<int16_t>();
			utils::serialize::any::add_serializer<int32_t>();
			utils::serialize::any::add_serializer<int64_t>();
			utils::serialize::any::add_serializer<uint8_t>();
			utils::serialize::any::add_serializer<uint16_t>();
			utils::serialize::any::add_serializer<uint32_t>();
			utils::serialize::any::add_serializer<uint64_t>();
			utils::serialize::any::add_serializer<std::string>();
		});
	}
}
