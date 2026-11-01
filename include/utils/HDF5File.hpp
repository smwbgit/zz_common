#pragma once

#include "hdf5.h"
#include "hdf5_hl.h"
#include "utils/ManagedHandle.hpp"
#include "utils/real_path.hpp"
#include "utils/computer_basic.hpp"
#include "utils/string.hpp"
#include "utils/type_traits.hpp"
#include "utils/Date.hpp"
#include "utils/Point3D.hpp"
#include "utils/iterator.hpp"

namespace utils
{
	namespace inner
	{
		//hdf5文件的基本读写
		class HDF5FileBase
		{
		private:
		
	#pragma region Id
			struct CloseGroupId
			{
				void operator()(hid_t id)
				{
					auto err = H5Gclose(id);
					utils_assert(succeed(err));
				}
			};
			using GroupId = ManagedHandle<hid_t, CloseGroupId>;

			struct CloseDatasetId
			{
				void operator()(hid_t id)
				{
					auto err = H5Dclose(id);
					utils_assert(succeed(err));
				}
			};
			using DatasetId = ManagedHandle<hid_t, CloseDatasetId>;

			struct CloseAttributeId
			{
				void operator()(hid_t id)
				{
					auto err = H5Aclose(id);
					utils_assert(succeed(err));
				}
			};
			using AttributeId = ManagedHandle<hid_t, CloseAttributeId>;

			struct CloseTypeId
			{
				void operator()(hid_t id)
				{
					auto err = H5Tclose(id);
					utils_assert(succeed(err));
				}
			};
			using TypeId = ManagedHandle<hid_t, CloseTypeId>;

			struct CloseDataspaceId
			{
				void operator()(hid_t id)
				{
					auto err = H5Sclose(id);
					utils_assert(succeed(err));
				}
			};
			using DataspaceId = ManagedHandle<hid_t, CloseDataspaceId>;

	#pragma endregion
		public:
			enum class Mode
			{
				create,	//如果存在，则清空内容
				open,	//打开一个已存在的
			};

			HDF5FileBase(const std::string & filename, Mode mode)
			{
				switch(mode)
				{
				case Mode::create:
					init_create(filename);
					break;
				case Mode::open:
					init_open(filename);
					break;
				default:
					utils_throw();
				}
			}

#pragma region group
			//创建group，如果不存在。包括路径上缺失的group
			void create_groups(_In_ const std::string & path)
			{
				std::vector<std::string> groups = split_to_root(path);
				utils_assert(groups.empty() == false);

				for (const auto & group : groups)
				{
					if (exist_group(group))
						continue;

					create_group(group);
				}
			}
			//创建group
			void create_group(_In_ const std::string & path)
			{
				GroupId id = H5Gcreate(m_file, path.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
				utils_assert(succeed(id));
			}
			//group是否存在
			bool exist_group(_In_ const std::string & path)
			{
				H5G_stat_t statbuf{};
				auto err = H5Gget_objinfo(m_file, path.c_str(), 0, &statbuf);
				if (succeed(err) == false)
					return false;

				utils_assert(statbuf.type == H5G_GROUP);
				return true;
			}
			//一个group里有多少group和dataset，不包括子group里的
			uint64_t get_children_count(_In_ const std::string & path)
			{
				H5G_info_t info{};
				auto err = H5Gget_info_by_name(m_file, path.c_str(), &info, H5P_DEFAULT);
				utils_assert(succeed(err));
				return info.nlinks;
			}
#pragma endregion
#pragma region dataset
			//dataset是否存在
			bool exist_dataset(_In_ const std::string & path)
			{
				H5G_stat_t statbuf{};
				auto err = H5Gget_objinfo(m_file, path.c_str(), 0, &statbuf);
				if (succeed(err) == false)
					return false;

				utils_assert(statbuf.type == H5G_DATASET);
				return true;
			}
			//移除dataset
			void remove_dataset(_In_ const std::string & path)
			{
				//简单检查
				utils_assert(exist_dataset(path));

				//删除
				auto err = H5Ldelete(m_file, path.c_str(), H5P_DEFAULT);
				utils_assert(succeed(err));
			}
			//写dataset    非字符串
			template<typename T, typename vector_T>
			void write_dataset(const std::string & path, const vector_T & values)
			{
				//简单检查
				static_assert(
					std::is_same_v<T, uint8_t> ||
					std::is_same_v<T, int> ||
					std::is_same_v<T, float> ||
					std::is_same_v<T, double>, "");

				//创建所在目录
				create_its_groups(path);

				//删掉旧有的dataset
				if (exist_dataset(path))
					remove_dataset(path);

				//创建dataset
				DatasetId dataset;
				create_dataset<T>(dataset, path, values);

				//写入数据
				fill_dataset<T>(dataset, values);
			}
			//特化
			template<typename T>
			void write_dataset(const std::string & path, const std::vector<T> & values)
			{
				write_dataset<T, std::vector<T>>(path, values);
			}
			//特化
			template<typename T>
			void write_dataset(const std::string & path, const std::vector<std::vector<std::vector<T>>> & values)
			{
				write_dataset<T, std::vector<std::vector<std::vector<T>>>>(path, values);
			}
			//写dataset    字符串
			//reserved_size：开辟dataset时每个字符串保留多大的空间
			void write_string_dataset(const std::string & path, const std::vector<std::string> & values, uint64_t reserved_size)
			{
				//创建所在目录
				create_its_groups(path);

				//删掉旧有的dataset
				if (exist_dataset(path))
					remove_dataset(path);

				//创建dataset
				DatasetId dataset;
				create_string_dataset(dataset, path, values, reserved_size);

				//写入数据
				fill_string_dataset(dataset, values, reserved_size);
			}
			//读dataset
			template<typename T>
			void read_dataset(std::vector<T> & values, const std::string & path)
			{
				//简单检查
				static_assert(
					std::is_same_v<T, uint8_t> ||
					std::is_same_v<T, float>, "");

				//打开dataset
				DatasetId dataset = H5Dopen(m_file, path.c_str(), H5P_DEFAULT);
				utils_assert(succeed(dataset));

				//打开dataspace
				DataspaceId dataspace = H5Dget_space(dataset);
				utils_assert(succeed(dataspace));

				//简单检查
				int rank = H5Sget_simple_extent_ndims(dataspace);
				utils_assert(succeed(rank));
				utils_assert(rank == 1);		//确实在读一个一维数组

				//查询元素个数和元素大小
				hsize_t dims{};
				int ndims = H5Sget_simple_extent_dims(dataspace, &dims, NULL);
				utils_assert(succeed(ndims));
				TypeId datatype = H5Dget_type(dataset);
				utils_assert(succeed(datatype));
				std::size_t datatype_size = H5Tget_size(datatype);
				utils_assert(datatype_size != 0);	//H5Tget_size()失败时为0

				//简单检查
				utils_assert(datatype_size == sizeof(T));

				//读取
				values.resize(dims);
				auto err = H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
				utils_assert(succeed(err));
			}
#pragma endregion
		
			//对象类型
			enum class ObjectType
			{
				group,
				dataset,
			};
			//获得path指向的对象类型
			//note：如果对象不存在，出错
			ObjectType get_object_type(const std::string & path)
			{
				H5G_stat_t statbuf{};
				auto err = H5Gget_objinfo(m_file, path.c_str(), 0, &statbuf);
				utils_assert(succeed(err));

				switch(statbuf.type)
				{
				case H5G_GROUP:
					return ObjectType::group;
				case H5G_DATASET:
					return ObjectType::dataset;
				}
				utils_throw();
			}

#pragma region attribute
			//添加属性
			//T: 属性类型
			template<typename T, typename From_T>
			void add_attribute(const std::string & path, const std::string & name, const From_T & value)
			{
				static_assert(
					std::is_same_v<T, int32_t> ||
					std::is_same_v<T, float> ||
					std::is_same_v<T, std::string>, "");
			
				switch (get_object_type(path))	//不存在也utils_throw()
				{
				case ObjectType::group:
				{
					GroupId parent;
					open_group(parent, path);
					return _add_attribute<T>(parent, name, value);
				}
				case ObjectType::dataset:	//table也能用open_dataset()打开
				{
					DatasetId parent;
					open_dataset(parent, path);
					return _add_attribute<T>(parent, name, value);
				}
				default:
					utils_throw();
				}
			}
			//读取属性
			//T: 属性类型
			template<typename T>
			void read_attribute(T & value, const std::string & path)
			{
				//简单检查
				static_assert(
					std::is_same_v<T, int32_t> ||
					std::is_same_v<T, float>, "");

				//打开属性
				auto pair = split_to_parent_and_it(path);
				AttributeId attr_id = H5Aopen_by_name(m_file, pair.first.c_str(), pair.second.c_str(), H5P_DEFAULT, H5P_DEFAULT);
				utils_assert(succeed(attr_id));
//				//打开属性
//				AttributeId attr_id = H5Aopen(m_file, path.c_str(), H5P_DEFAULT);
//				utils_assert(succeed(attr_id));

				//获取大小
				H5A_info_t info{};
				auto err = H5Aget_info(attr_id, &info);
				utils_assert(succeed(err));
				auto size = info.data_size;

				//简单检查
				utils_assert(size == sizeof(T));

				//获取类型
				TypeId type_id = H5Aget_type(attr_id);
				utils_assert(succeed(type_id));

				//读取
				err = H5Aread(attr_id, type_id, &value);
				utils_assert(succeed(err));
			}
			//特化
			void read_attribute(std::string & value, const std::string & path)
			{
				//打开属性
				auto pair = split_to_parent_and_it(path);
				AttributeId attr_id = H5Aopen_by_name(m_file, pair.first.c_str(), pair.second.c_str(), H5P_DEFAULT, H5P_DEFAULT);
				utils_assert(succeed(attr_id));
//				//打开属性
//				AttributeId attr_id = H5Aopen(m_file, path.c_str(), H5P_DEFAULT);
//				utils_assert(succeed(attr_id));

				//获取大小
				H5A_info_t info{};
				auto err = H5Aget_info(attr_id, &info);
				utils_assert(succeed(err));
				auto size = info.data_size;

				//设置长度
				value.resize(size);

				//获取类型
				TypeId type_id = H5Aget_type(attr_id);
				utils_assert(succeed(type_id));

				//读取
				err = H5Aread(attr_id, type_id, (char*)value.c_str());
				utils_assert(succeed(err));

				//简单检查
				utils_assert(value[size - 1] != 0);
			}
			//获得指定group或者dataset上的属性个数
			uint64_t get_attribute_n(const std::string & path)
			{
				H5O_info_t object_info{};
				auto err = H5Oget_info_by_name(m_file, path.c_str(), &object_info, H5P_DEFAULT);
				utils_assert(succeed(err));
				return object_info.num_attrs;
			}
#pragma endregion
			
#pragma region table
			//表是否存在
			bool exist_table(const std::string & path) const
			{
				hsize_t nfields{};
				hsize_t nrecords{};
				auto err = H5TBget_table_info(m_file, path.c_str(), &nfields, &nrecords);
				return succeed(err);
			}
			//删除表
			void remove_table(const std::string & path) const
			{
				//简单检查
				utils_assert(exist_table(path));

				//删除
				auto err = H5Ldelete(m_file, path.c_str(), H5P_DEFAULT);
				utils_assert(succeed(err));
			}
			//表的列
			class Columns
			{
			public:
				//添加列
				template<typename T>
				void add(const std::string & column_name)
				{
					//简单检查
					static_assert(std::is_same_v<T, std::string> == false, "");
					static_assert(utils::is_arithmetic_v<T>, "");

					//准备offset
					uint64_t offset = m_cur_offset;
					m_cur_offset += sizeof(T);

					//准备type_id
					TypeId type_id;
					get_type_id<T>(type_id);

					//保存
					m_columns.push_back(Column{column_name, offset, std::move(type_id)});
				}
				//特化
				template<>
				void add<utils::Date>(const std::string & column_name)
				{
					//日期类型变为用字符串存储，长度11
					add<std::string>(column_name, 11);
				}
				//特化
				template<typename T>
				void add(const std::string & column_name, uint64_t size)
				{
					//简单检查
					static_assert(std::is_same_v<T, std::string>, "");

					//准备offset
					uint64_t offset = m_cur_offset;
					m_cur_offset += size;

					//准备type_id
					TypeId type_id;
					get_type_id<std::string>(type_id, size);

					//保存
					m_columns.push_back(Column{column_name, offset, std::move(type_id)});
				}
				//获得所有列
				void get(_Out_ hsize_t & nfields, _Out_ size_t & type_size,
					_Out_ std::unique_ptr<const char*[]> & field_names,
					_Out_ std::unique_ptr<size_t[]> & field_offset,
					_Out_ std::unique_ptr<hid_t[]> & field_types) const
				{
					//简单检查
					utils_assert(m_columns.empty() == false);

					nfields = m_columns.size();
					type_size = m_cur_offset;

					field_names.reset(new const char*[nfields]{});
					field_offset.reset(new size_t[nfields]{});
					field_types.reset(new hid_t[nfields]{});

					for (std::size_t i = 0; i < m_columns.size(); i++)
					{
						field_names[i]	= m_columns[i].name.c_str();
						field_offset[i]	= m_columns[i].offset;
						field_types[i]	= m_columns[i].type_id;
					}
				}
			private:
				struct Column
				{
					std::string name;
					uint64_t offset;
					TypeId type_id;
				};
				std::vector<Column> m_columns;
				uint64_t m_cur_offset{};
			};
			class Table
			{
			public:
				~Table()
				{
					//缓冲写入文件
					flush_rows();
				}
				//插入一行
				template<typename... Args>
				void append_row(Args && ... args)
				{
					//使用append_row_value()添加值
					//使用append_row_value_finish()写入

					_append_row(std::forward<Args>(args)...);
				}
			
				//写入一行里的一个值
				template<typename T>
				void append_row_value(const T & value)
				{
					//简单检查
					static_assert(utils::is_arithmetic_v<T>, "");
					utils_assert(m_row_element_index < m_field_sizes.size());
					utils_assert(sizeof(T) == m_field_sizes[m_row_element_index]);

					//写入缓冲
					memmove(&m_row[m_field_offsets[m_row_element_index]], &value, sizeof(T));
					m_row_element_index++;
				}
				//特化
				void append_row_value(const utils::Point3D & pt)
				{
					append_row_value(pt.x);
					append_row_value(pt.y);
					append_row_value(pt.z);
				}
				//特化
				void append_row_value(const std::string & value)
				{
					//简单检查
					utils_assert(m_row_element_index < m_field_sizes.size());
					utils_assert(value.length() + 1 <= m_field_sizes[m_row_element_index]);

					//写入缓冲
					memmove(&m_row[m_field_offsets[m_row_element_index]], value.c_str(), value.length() + 1);
					m_row_element_index++;
				}
				//特化
				void append_row_value(const utils::Date & date)
				{
					//转为字符串
					std::string text;
					date.to_string(text);

					//简单检查
					utils_assert(text.length() == 10);

					//写入缓冲
					append_row_value(text);
				}
				//特化: 写入一行里的多个值
				template<typename T>
				void append_row_value(const std::vector<T> & vec)
				{
					//简单检查
					static_assert(utils::is_arithmetic_v<T>, "");

					for (const auto & value : vec)
						append_row_value(value);
				}
				//特化: 写入一行里的多个值
				template<typename T, std::size_t array_size, std::enable_if_t<utils::is_arithmetic_v<T> == false, int> EnableIf = 0>
				void append_row_value(const std::array<T, array_size> & vec)
				{
					//循环写入
					for (std::size_t i = 0; i < array_size; i++)
						append_row_value(vec[i]);
				}
				//写入行
				void append_row_value_finish()
				{
					//简单检查
					utils_assert(m_row_element_index == m_field_sizes.size());

					//写入缓冲
					memmove(&m_rows[m_rows_n * m_type_size], m_row.data(), m_type_size);
					m_rows_n++;

					//写入
					if (m_rows_n == m_rows_max_n)
						flush_rows();

					//重置
					m_row_element_index = 0;
				}
	#if 0
				//写入一行里的一个值
				template<typename T>
				void append_row_value(const T & value)
				{
					//简单检查
					static_assert(utils::is_arithmetic_v<T>, "");
					utils_assert(m_row_pos + sizeof(T) <= m_type_size);

					//写入缓冲
					memmove(&m_row[m_row_pos], &value, sizeof(T));
					m_row_pos += sizeof(T);
				}
				//特化: 写入一行里的一个Point3D
				void append_row_value(const utils::Point3D & pt)
				{
					//简单检查
					utils_assert(m_row_pos + sizeof(pt) <= m_type_size);

					//写入缓冲
					memmove(&m_row[m_row_pos], &pt, sizeof(pt));
					m_row_pos += sizeof(pt);
				}
				//特化: 写入一行里的一个字符串
				void append_row_value(const std::string & value)
				{
					//简单检查
					utils_assert(m_row_pos + value.length() + 1 <= m_type_size);

					//写入缓冲
					memmove(&m_row[m_row_pos], value.c_str(), value.length() + 1);
					m_row_pos += value.length() + 1;
				}
				//特化: 写入一行里的一个日期
				void append_row_value(const utils::Date & date)
				{
					//转为字符串
					std::string text;
					date.to_string(text);

					//简单检查
					utils_assert(text.length() == 10);

					//写入缓冲
					append_row_value(text);
				}
				//特化: 写入一行里的多个值
				template<typename T>
				void append_row_value(const std::vector<T> & vec)
				{
					//简单检查
					static_assert(utils::is_arithmetic_v<T>, "");
					utils_assert(m_row_pos + sizeof(T) * vec.size() <= m_type_size);

					//写入缓冲
					memmove(&m_row[m_row_pos], vec.data(), sizeof(T) * vec.size());
					m_row_pos += sizeof(T) * vec.size();
				}
				//特化: 写入一行里的多个值
				template<typename T, std::size_t array_size, std::enable_if_t<utils::is_arithmetic_v<T> == false, int> EnableIf = 0>
				void append_row_value(const std::array<T, array_size> & vec)
				{
					//循环写入
					for (std::size_t i = 0; i < array_size; i++)
						append_row_value(vec[i]);
				}
				//写入行
				void append_row_value_finish()
				{
					//简单检查
					utils_assert(m_row_pos == m_type_size);

					//写入缓冲
					memmove(&m_rows[m_rows_n * m_type_size], m_row.data(), m_type_size);
					m_rows_n++;

					//写入
					if (m_rows_n == m_rows_max_n)
						flush_rows();

					//重置
					m_row_pos = 0;
				}
	#endif
				//写入缓冲的多行
				void flush_rows()
				{
					if (m_rows_n == 0)
						return;

					//写入
					auto err = H5TBappend_records(m_parent, m_name.c_str(), m_rows_n, m_type_size, m_field_offsets.data(), m_field_sizes.data(), m_rows.data());
					utils_assert(succeed(err));

					//重置
					m_rows_n = 0;

					//更新行数列数
					update_table_size();
				}
				//读取全部行
				template<typename T>
				void get_rows(std::vector<T> & data)
				{
					//刷新缓冲
					flush_rows();

					//简单检查
					utils_assert(sizeof(T) == m_type_size);

					//读取
					hsize_t start = 0;
					data.resize(m_row_n);
					err = H5TBread_records(m_parent, m_name.c_str(), start, m_row_n, m_type_size, m_field_offsets.data(), m_field_sizes.data(), data.data());
					utils_assert(succeed(err));
				}
				//获得行数
				uint64_t get_row_n()
				{
					return m_row_n;
				}
				//获得列数
				uint64_t get_column_n()
				{
					return m_column_n;
				}
				//获得一行
				void get_one_row(std::vector<uint8_t> & data, uint64_t row_index)
				{
					//刷新缓冲
					flush_rows();

					//简单检查
					utils_assert(row_index < m_row_n);

					//读取
					data.resize(m_type_size);
					auto err = H5TBread_records(m_parent, m_name.c_str(), row_index, 1, m_type_size, m_field_offsets.data(), m_field_sizes.data(), data.data());
					utils_assert(succeed(err));
				}
				//获得多行
				//row_begin：起始行
				//row_n：行数
				//is_check_row_n: 是否row_begin+row_n可以比m_row_n还大
				template<bool is_check_row_n = true>
				void get_some_rows(std::vector<uint8_t> & rows, uint64_t row_begin, uint64_t row_n)
				{
					//刷新缓冲
					flush_rows();

					//简单检查
					if (is_check_row_n)
						utils_assert(row_begin + row_n <= m_row_n);
					else
					{
						if (row_begin + row_n > m_row_n)
						{
							utils_assert(row_begin < m_row_n);
							row_n = m_row_n - row_begin;
							utils_assert(row_n > 0);
						}
					}

					//读取
					rows.resize(m_type_size * row_n);
					auto err = H5TBread_records(m_parent, m_name.c_str(), row_begin, row_n, m_type_size, m_field_offsets.data(), m_field_sizes.data(), rows.data());
					utils_assert(succeed(err));
				}
				//获得一行的大小
				uint64_t get_row_size() const
				{
					return m_type_size;
				}
			private:
				//构造函数
				Table(_In_ GroupId && parent, _In_ const std::string & name, _In_ const std::string & parent_text)
					: m_parent{std::move(parent)}
					, m_name{name}
					, m_parent_text{parent_text}
				{
					//获得行数列数
					update_table_size();

					//保存字段大小和偏移
					m_field_sizes.resize(m_column_n);
					m_field_offsets.resize(m_column_n);
					auto err = H5TBget_field_info(m_parent, m_name.c_str(), nullptr, m_field_sizes.data(), m_field_offsets.data(), &m_type_size);
					utils_assert(succeed(err));
					utils_assert(m_type_size > 0);

					//开辟行缓存
					m_row.resize(m_type_size);

					//开辟多行缓存
					const uint64_t transfer_threshold_size = 10 * 1000 * 1000;	//10M
					m_rows_max_n = transfer_threshold_size / m_type_size;
					utils_assert(m_rows_max_n > 0);
					m_rows.resize(m_rows_max_n * m_type_size);
				}
				//更新行数列数
				void update_table_size()
				{
					auto err = H5TBget_table_info(m_parent, m_name.c_str(), &m_column_n, &m_row_n);
					utils_assert(succeed(err));
				}
				//插入一行: 具体实现
				template<typename T, typename... Args>
				void _append_row(T && value, Args && ... args)
				{
					append_row_value(std::forward<T>(value));
					_append_row(std::forward<Args>(args)...);
				}
				//特化
				void _append_row()
				{
					append_row_value_finish();
				}

				friend class utils::inner::HDF5FileBase;
				GroupId m_parent;
				std::string m_name;
				std::string m_parent_text;		//for debug
				size_t m_type_size{};					//表头信息
				std::vector<size_t> m_field_offsets;	//表头信息
				std::vector<size_t> m_field_sizes;		//表头信息
				std::vector<uint8_t> m_row;			//行缓冲
				uint64_t m_row_element_index{};		//行缓冲的当前位置
				std::vector<uint8_t> m_rows;		//多行缓冲
				uint64_t m_rows_n{};				//多行缓冲的当前已有的个数
				uint64_t m_rows_max_n{};			//多行缓冲的最大个数
				hsize_t m_column_n{};	//列数
				hsize_t m_row_n{};		//行数 (不包括在缓冲中的)
			};

			//创建表
			//columns: 列定义
			std::unique_ptr<Table> create_table(_In_ const std::string & path, _In_ const Columns & columns)
			{
				//分割path
				std::string parent;
				std::string it;
				std::tie(parent, it) = split_to_parent_and_it(path);

				//打开所在目录
				create_groups(parent);
				GroupId parent_id;
				open_group(parent_id, parent);

				//准备参数
				hsize_t nfields{};
				size_t type_size{};
				std::unique_ptr<const char*[]> field_names;
				std::unique_ptr<size_t[]> field_offset;
				std::unique_ptr<hid_t[]> field_types;
				columns.get(nfields, type_size, field_names, field_offset, field_types);

				//创建
				auto err = H5TBmake_table(it.c_str(), parent_id, it.c_str(), nfields, 0, type_size, field_names.get(), field_offset.get(), field_types.get(), type_size, NULL, 0, NULL);
				utils_assert(succeed(err));

				//返回一个Table
				return std::unique_ptr<Table>{new Table{std::move(parent_id), it, parent}};
			}

			//打开表
			std::unique_ptr<Table> open_table(const std::string & path)
			{
				//分割path
				std::string parent;
				std::string it;
				std::tie(parent, it) = split_to_parent_and_it(path);

				//打开所在目录
				GroupId parent_id;
				open_group(parent_id, parent);

				return std::unique_ptr<Table>{new Table{std::move(parent_id), it, parent}};
			}

			class TableheadInfo
			{
			public:
				std::vector<std::string> names;
			};
			//获得表头信息
			void get_tablehead_info(_Out_ TableheadInfo & info, const std::string & path)
			{
				//获得字段数量
				hsize_t nfields{};
				hsize_t nrecords{};
				auto err = H5TBget_table_info(m_file, path.c_str(), &nfields, &nrecords);
				utils_assert(succeed(err));

				//保存字段名字
				std::vector<std::array<char, 255>> buffer(nfields);	//todo 确认下255会不会溢出，或者找个办法确认出最大长度
				std::vector<char*> __names;
				for (auto & n : buffer)
					__names.push_back(&n[0]);
				err = H5TBget_field_info(m_file, path.c_str(), __names.data(), nullptr, nullptr, nullptr);
				utils_assert(succeed(err));
				info.names.assign(__names.begin(), __names.end());
			}

#pragma region iterator_table
			
			class iterator_table__entry
			{
			public:
				//是否已获取完全部列
				bool is_readed_all() const
				{
					return m_column_index >= m_table.get_column_n();
				}
				//获得一个值，并把指针指向下一个
				template<typename T>
				void get_value(T & value) const
				{
					//简单检查
					utils_assert(m_column_index < m_table.get_column_n());
					const auto field_size = m_table.m_field_sizes[m_column_index];
					utils_assert(sizeof(T) == field_size);

					//读取
					const auto field_offset = m_table.m_field_offsets[m_column_index];
					memmove(&value, &m_line_data[field_offset], field_size);

					//更新状态
					m_column_index++;
				}
				//特化
				template<>
				void get_value<std::string>(std::string & value) const
				{
					//简单检查
					utils_assert(m_column_index < m_table.get_column_n());

					//读取
					const auto field_size = m_table.m_field_sizes[m_column_index];
					value.resize(field_size);
					const auto field_offset = m_table.m_field_offsets[m_column_index];
					memmove((void*)value.data(), &m_line_data[field_offset], field_size);
					value = value.c_str();

					//简单检查
					utils_assert(field_size - value.length() <= 1);	//最多多出来一个\0
					if (value.empty() == false)
						utils_assert(value[value.length() - 1] != 0);	//多出来的\0已被去掉

					//更新状态
					m_column_index++;
				}
				iterator_table__entry(const uint8_t* line_data, Table & table)
					: m_table{table}
					, m_line_data{line_data}
				{
//					table.get_one_row(m_line_data, m_row_index);
				}
			private:
				Table & m_table;
				mutable uint64_t m_column_index{};
				const uint8_t* m_line_data;
			};
			class iterator_table__iterator
			{
			public:
				iterator_table__iterator(Table & table, uint64_t row_index)
					: m_table{table}
					, m_row_index{row_index}
				{

				}
				bool operator != (const iterator_table__iterator & o) const
				{
					return m_row_index != o.m_row_index;
				}
				void operator++ ()
				{
					utils_assert(m_row_index < m_table.get_row_n());
					m_row_index++;
				}
				iterator_table__entry operator*()
				{
					const uint8_t* line_data{};
					get_line_data_from_buffer(line_data, m_row_index);

					return {line_data, m_table};
				}
			private:
				void get_line_data_from_buffer(const uint8_t* & line_data, uint64_t row_index)
				{
					//查看缓冲，如果有，则将其返回，否则释放之前的缓存，查询当前的

					if (m_rows.empty() == false		//有缓冲数据
						&& m_row_begin <= row_index && row_index < m_row_begin + m_row_n	//要取的数据在缓冲里
						)
					{
						line_data = &m_rows[(row_index - m_row_begin) * m_table.get_row_size()];
						return;
					}

					m_rows.clear();
					m_row_begin = row_index - row_index % m_row_n;
					m_table.get_some_rows<false>(m_rows, m_row_begin, m_row_n);
					line_data = &m_rows[(row_index - m_row_begin) * m_table.get_row_size()];
				}
				Table & m_table;
				uint64_t m_row_index{};
				
				const uint64_t m_row_n = 100;	//100行缓冲一次
				uint64_t m_row_begin{};			//缓冲数据的起始行
				std::vector<uint8_t> m_rows;	//缓冲数据
			};
			class iterator_table__iterators
			{
			public:
				iterator_table__iterator begin()
				{
					return {*m_table.get(), 0};
				}
				iterator_table__iterator end()
				{
					return {*m_table.get(), m_table->get_row_n()};
				}
			private:
				iterator_table__iterators(HDF5FileBase & hdf, const std::string & path)
				{
					m_table = hdf.open_table(path);
				}
				friend class HDF5FileBase;
				std::unique_ptr<Table> m_table;
			};
			//遍历表的每一行
			iterator_table__iterators iterator_table(const std::string & path)
			{
				return {*this, path};
			}
#pragma endregion
#pragma endregion
		protected:

			//分割路径/a/b/c/ 为 /a  /a/b  /a/b/c
			static std::vector<std::string> split_to_root(_In_ const std::string & path)
			{
				//分割
				std::vector<std::string> parts;
				utils::string_function::split(parts, path, '/', false);

				//组合
				std::vector<std::string> re;
				std::string cur;
				for (const auto & part : parts)
				{
					cur += '/' + part;
					re.push_back(cur);
				}
				return re;
			}

		private:

			void init_open(const std::string & filename)
			{
				//简单检查
				utils_assert(m_filename.empty());

				m_filename = filename;
				m_file = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
			}
			void init_create(const std::string & filename)
			{
				//简单检查
				utils_assert(m_filename.empty());

				//创建目录
				utils::real_path::create_its_directory_if_not_exist(filename);

				m_filename = filename;
				m_file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
			}
			static bool succeed(hid_t id)
			{
				return id >= 0;
			}
			//分割路径/a/b/c/d 为 /a/b/c 和 d
			static std::pair<std::string, std::string> split_to_parent_and_it(_In_ const std::string & path)
			{
				//分割
				std::vector<std::string> parts;
				utils::string_function::split(parts, path, '/', false);
			
				//简单检查
				utils_assert(parts.empty() == false);

				//组合
				std::string parent;
				for (std::size_t i = 0; i < parts.size() - 1; i++)
				{
					parent += '/' + parts[i];
				}
				if (parent.empty())
					parent = '/';

				const auto & it = *parts.rbegin();

				return std::make_pair(parent, it);
			}
			//打开group
			void open_group(_Out_ GroupId & id, _In_ const std::string & path)
			{
				utils_assert(try_open_group(id, path));
			}
			//试着打开group
			bool try_open_group(_Out_ GroupId & id, _In_ const std::string & path)
			{
				auto _id = H5Gopen(m_file, path.c_str(), H5P_DEFAULT);
				if (succeed(_id))
				{
					id = _id;
					return true;
				}
				else
					return false;
			}
			//试着打开dataset
			bool try_open_dataset(_Out_ DatasetId & id, _In_ const std::string & path)
			{
				auto _id = H5Dopen(m_file, path.c_str(), H5P_DEFAULT);
				if (succeed(_id))
				{
					id = _id;
					return true;
				}
				else
					return false;
			}
			//打开dataset
			void open_dataset(_Out_ DatasetId & id, _In_ const std::string & path)
			{
				utils_assert(try_open_dataset(id, path));
			}
			//dataset是否存在
			bool exist_dataset(_In_ const GroupId & path, _In_ const std::string & name)
			{
				H5G_stat_t statbuf{};
				auto err = H5Gget_objinfo(path, name.c_str(), 0, &statbuf);
				if (succeed(err) == false)
					return false;

				utils_assert(statbuf.type == H5G_DATASET);
				return true;
			}
			//试着打开dataset
			bool try_open_dataset(_Out_ DatasetId & id, _In_ const GroupId & path, _In_ const std::string & name)
			{
				auto _id = H5Dopen(path, name.c_str(), H5P_DEFAULT);
				if (succeed(_id))
				{
					id = _id;
					return true;
				}
				else
					return false;
			}
			//打开dataset
			void open_dataset(_Out_ DatasetId & id, _In_ const GroupId & path, _In_ const std::string & name)
			{
				utils_assert(try_open_dataset(id, path, name));
			}

			//attribute是否存在
			template<typename Id_T>
			bool exist_attribute(_In_ const Id_T & path, _In_ const std::string & name)
			{
				//简单检查
				static_assert(
					std::is_same_v<Id_T, GroupId> ||
					std::is_same_v<Id_T, DatasetId>, "");

				//判断是否存在
				auto re = H5Aexists(path, name.c_str());
				utils_assert(succeed(re));
				return re > 0;
			}
			//试着打开attribute
			bool try_open_attribute(_Out_ AttributeId & id, _In_ const GroupId & path, _In_ const std::string & name)
			{
				auto _id = H5Aopen(path, name.c_str(), H5P_DEFAULT);
				if (succeed(_id))
				{
					id = _id;
					return true;
				}
				else
					return false;
			}
			//打开attribute
			void open_attribute(_Out_ AttributeId & id, _In_ const GroupId & path, _In_ const std::string & name)
			{
				utils_assert(try_open_attribute(id, path, name));
			}

			//移除dataset
			void remove_dataset(_In_ const GroupId & path, _In_ const std::string & name)
			{
				//简单检查
				utils_assert(exist_dataset(path, name));

				//删除
				auto err = H5Ldelete(path, name.c_str(), H5P_DEFAULT);
				utils_assert(succeed(err));
			}
		
			//获得类型id
			template<typename T>
			static void get_type_id(_Out_ TypeId & id)
			{
				static_assert(false, "");
			}
			//特化
			template<>
			static void get_type_id<uint8_t>(_Out_ TypeId & id)
			{
				id = H5Tcopy(H5T_NATIVE_B8);
				utils_assert(succeed(id));
			}
			//特化
			template<>
			static void get_type_id<float>(_Out_ TypeId & id)
			{
				id = H5Tcopy(H5T_NATIVE_FLOAT);
				utils_assert(succeed(id));
			}
			//特化
			template<>
			static void get_type_id<double>(_Out_ TypeId & id)
			{
				id = H5Tcopy(H5T_NATIVE_DOUBLE);
				utils_assert(succeed(id));
			}
			//特化
			template<>
			static void get_type_id<int>(_Out_ TypeId & id)
			{
				id = H5Tcopy(H5T_NATIVE_INT);
				utils_assert(succeed(id));
			}
			//特化
			template<>
			static void get_type_id<uint32_t>(_Out_ TypeId & id)
			{
				id = H5Tcopy(H5T_NATIVE_UINT);
				utils_assert(succeed(id));
			}
			//获得类型id: 对于字符串
			template<typename T>
			static void get_type_id(_Out_ TypeId & id, _In_ const std::string & text)
			{
				get_type_id<T>(id, text.length() + 1);
			}
			//获得类型id: 对于字符串
			//note: size为字符串长度+1
			template<typename T>
			static void get_type_id(_Out_ TypeId & id, _In_ uint64_t size)
			{
				//从H5T_C_S1继承，然后设置其大小

				static_assert(std::is_same_v<T, std::string>, "");

				//继承
				id = H5Tcopy(H5T_C_S1);
				utils_assert(succeed(id));

				//设置大小
				auto err = H5Tset_size(id, size);
				utils_assert(succeed(err));

	//			auto aaa = H5Tget_size(id);
	//			auto aaa = H5Tget_size(id);
			
	//			H5T_pad_t lsb{};
	//			H5T_pad_t msb{};
	//			auto err1 = H5Tget_pad(id, &lsb, &msb);
	//			throw std::make_tuple(err1,lsb,msb,aaa);
				//设置填充方式
	//			err = H5Tset_strpad(id, H5T_STR_NULLTERM);
	//			utils_assert(succeed(err));
			}
			//获得数据空间id    标量
			void get_dataspace_id(_Out_ DataspaceId & id)
			{
				//创建数据空间
				id = H5Screate(H5S_SCALAR);	

				//简单检查
				utils_assert(succeed(id));
			}
			//获得数据空间id    数组
			template<typename T>
			void get_dataspace_id(_Out_ DataspaceId & id, _In_ const std::vector<T> & vec)
			{
				//创建数据空间
				hsize_t dims[1] = {vec.size()};
				id = H5Screate_simple(1, dims, NULL);

				//简单检查
				utils_assert(succeed(id));
			}
			//特化
			template<typename T>
			void get_dataspace_id(_Out_ DataspaceId & id, _In_ const std::vector<std::vector<std::vector<T>>> & vec)
			{
				//简单检查
				utils_assert(vec.empty()==false);
				utils_assert(vec[0].empty()==false);
				utils_assert(vec[0][0].empty()==false);

				//创建数据空间
				hsize_t dims[3] = {vec.size(), vec[0].size(), vec[0][0].size()};
				id = H5Screate_simple(3, dims, NULL);

				//简单检查
				utils_assert(succeed(id));
			}
			//创建数据集      非字符串
			//note: 如果已存在则失败
			template<typename T, typename vector_T>
			void create_dataset(
				_Out_ DatasetId & id,
				_In_ const std::string & path,
				_In_ const vector_T & vec)
			{
				//获得类型id
				TypeId type_id;
				get_type_id<T>(type_id);

				//获得数据空间id
				DataspaceId dataspace_id;
				get_dataspace_id(dataspace_id, vec);

				//创建数据集
				id = H5Dcreate(m_file, path.c_str(), type_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

				//简单检查
				utils_assert(succeed(id));
			}
			//创建数据集      字符串
			//note: 如果已存在则失败
			void create_string_dataset(
				_Out_ DatasetId & id,
				_In_ const std::string & path,
				_In_ const std::vector<std::string> & vec,
				_In_ uint64_t reserved_size)
			{
				//获得类型id
				TypeId type_id;
				get_type_id<std::string>(type_id, reserved_size);

				//获得数据空间id
				DataspaceId dataspace_id;
				get_dataspace_id(dataspace_id, vec);

				//创建数据集
				id = H5Dcreate(m_file, path.c_str(), type_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

				//简单检查
				utils_assert(succeed(id));
			}
			//写入数据        非字符串
			template<typename T>
			void fill_dataset(_In_ const DatasetId & dataset_id, _In_ const std::vector<T> & values)
			{
				//获得类型id
				TypeId type_id;
				get_type_id<T>(type_id);

				//写入
				herr_t err = H5Dwrite(dataset_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data());
	
				//简单检查
				utils_assert(succeed(err));
			}
			//特化
			template<typename T>
			void fill_dataset(_In_ const DatasetId & dataset_id, _In_ const std::vector<std::vector<std::vector<T>>> & values)
			{
				//获得类型id
				TypeId type_id;
				get_type_id<T>(type_id);

				//准备数据
				//把数据都放在一维数组里
				auto n1 = values.size();
				auto n2 = values[0].size();
				auto n3 = values[0][0].size();
				std::vector<T> _values;
				_values.resize(n1 * n2 * n3);
				for (auto i : utils::iterator_index(n1))
				{
					//简单检查
					utils_assert(n2 == values[i].size());

					for (auto j : utils::iterator_index(n2))
					{
						//简单检查
						utils_assert(n3 == values[i][j].size());

						for (auto k : utils::iterator_index(n3))
						{
							_values[i * n2 * n3 + j * n3 + k] = values[i][j][k];
						}
					}
				}
				
				//写入
				herr_t err = H5Dwrite(dataset_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, _values.data());
	
				//简单检查
				utils_assert(succeed(err));
			}
			//写入数据        字符串
			void fill_string_dataset(_In_ const DatasetId & dataset_id, _In_ const std::vector<std::string> & values, _In_ uint64_t reserved_size)
			{
				//获得类型id
				TypeId type_id;
				get_type_id<std::string>(type_id, reserved_size);

				//准备数据
				std::vector<uint8_t> _values;
				_values.resize(values.size() * reserved_size);
				for (const auto & str : utils::iterator_and_give_me_i(values))
				{
					//简单检查
					utils_assert(str.value.length() + 1 <= reserved_size);

					memmove(&_values[str.i * reserved_size], str.value.c_str(), str.value.length() + 1);
				}

				//写入
				herr_t err = H5Dwrite(dataset_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, _values.data());
	
				//简单检查
				utils_assert(succeed(err));
			}
			//删除属性
			template<typename Id_T>
			void remove_attribute(const Id_T & parent, const std::string & name)
			{
				//简单检查
				static_assert(
					std::is_same_v<Id_T, GroupId> ||
					std::is_same_v<Id_T, DatasetId>, "");

				//删除
				auto err = H5Adelete(parent, name.c_str());
	
				//简单检查
				utils_assert(succeed(err));
			}
			
			//添加属性，具体实现
			template<typename T, typename From_T, typename Id_T, typename std::enable_if<std::is_same<T, std::string>::value == false, int>::type EnableIf = 0>
			void _add_attribute(const Id_T & parent, const std::string & name, const From_T & value)
			{
				//删除已有的
				if (exist_attribute(parent, name))
					remove_attribute(parent, name);

				//类型id
				TypeId type_id;
				get_type_id<T>(type_id);

				//数据空间id
				DataspaceId dataspace_id;
				get_dataspace_id(dataspace_id);

				//创建属性
				AttributeId id = H5Acreate(parent, name.c_str(), type_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
				utils_assert(succeed(id));

				//转换From_T 到 T
				T _value{};
				utils::computer_basic_function::store_this_integer(_value, value);

				//写入数据
				auto err = H5Awrite(id, type_id, &_value);
				utils_assert(succeed(err));
			}
		
			//特化
			template<typename T, typename From_T, typename Id_T, typename std::enable_if<std::is_same<T, std::string>::value, int>::type EnableIf = 0>
			void _add_attribute(const Id_T & parent, const std::string & name, const From_T & value)
			{
				//删除已有的
				if (exist_attribute(parent, name))
					remove_attribute(parent, name);

				//类型id
				TypeId type_id;
				get_type_id<std::string>(type_id, value);

				//数据空间id
				DataspaceId dataspace_id;
				get_dataspace_id(dataspace_id);

				//创建属性
				AttributeId id = H5Acreate(parent, name.c_str(), type_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT);
				utils_assert(succeed(id));

				//写入数据
				static_assert(std::is_same_v<T, From_T>, "");
				herr_t err = H5Awrite(id, type_id, value.c_str());
				utils_assert(succeed(err));
			}
			//创建所在目录
			//back_n: 回退次数
			void create_its_groups(const std::string & path, uint64_t back_n = 1)
			{
				if (back_n == 1)
				{
					auto pair = split_to_parent_and_it(path);
					create_groups(pair.first);
				}
				else
					utils_throw();
			}

			std::string m_filename;
			hid_t m_file{};
		};
	}

	//hdf5文件的读写
	class HDF5File : public utils::inner::HDF5FileBase
	{
	public:
		using utils::inner::HDF5FileBase::HDF5FileBase;
		using utils::inner::HDF5FileBase::TableheadInfo;
		using utils::inner::HDF5FileBase::Mode;

		//断言：某个attribute的值应该是什么
		//path: 路径
		//expected_value: 期望的attribute值
		template<typename T>
		void attribute_is_this(const std::string & path, const T & expected_value)
		{
			T value{};
			read_attribute(value, path);

			utils_assert(value == expected_value);
		}

		//断言：某个dataset的值应该是什么
		//path: dataset路径
		//expected_values: 期望的dataset值
		template<typename T>
		void dataset_is_this(const std::string & path, const std::vector<T> & expected_values)
		{
			std::vector<T> values;
			read_dataset(values, path);

			utils_assert(values == expected_values);
		}

		//断言：表头的内容
		void tablehead_is_this(_In_ const std::string & path, _In_ const std::vector<std::string> & column_names)
		{
			TableheadInfo info;
			get_tablehead_info(info, path);

			utils_assert(info.names == column_names);
		}
	};
}