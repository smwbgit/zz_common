#pragma once

//#include <sqlite3.h>	//sqlite版本 3.36.0
#include <string>
#include "utils/character_code.hpp"
#include "utils/assert.hpp"
#include "utils/CallWhenReturn.hpp"
#include "utils/type_traits.hpp"

#include "utils/compiler/windows_header/maxmin__eliminate.inc"

namespace utils
{
	//读取sqlite数据库
	class SqliteReader
	{
	public:
		//查询语句返回的一行结果
		class OneRowOfResult
		{
		public:
			//获得某一列的值
			//column_index: 列的位置.  从0开始
			template<typename T, typename std::enable_if<utils::is_integral<T>::value, int>::type unused = 0>
			void get_value(T & value, uint64_t column_index) const
			{
				//简单检查
				utils_assert(column_index < m_column_n);
				utils_assert(SQLITE_INTEGER == sqlite3_column_type(m_stmt, (int)column_index));

				//获取
				int64_t _value = sqlite3_column_int64(m_stmt, (int)column_index);
				
				//传出
				utils::computer_basic_function::store_this_integer(value, _value);
			}
			//特化
			void get_value(std::string & value, uint64_t column_index) const
			{
				//简单检查
				utils_assert(column_index < m_column_n);
				utils_assert(SQLITE_TEXT == sqlite3_column_type(m_stmt, (int)column_index));

				//获取
				auto value_utf8 = (const char*)sqlite3_column_text(m_stmt, (int)column_index);
				utils_assert(value_utf8 != nullptr);
				
				//转换并传出
				utils::character_code_function::convert_utf8_to_ansi(value, value_utf8);
			}
			//特化
			void get_blob_value(std::vector<uint8_t> & data, uint64_t column_index) const
			{
				//简单检查
				utils_assert(column_index < m_column_n);
				utils_assert(SQLITE_BLOB == sqlite3_column_type(m_stmt, (int)column_index));

				//获取
				auto _data = (const uint8_t*)sqlite3_column_blob(m_stmt, (int)column_index);
				auto data_size = sqlite3_column_bytes(m_stmt, (int)column_index);		//官方文档建议：先 get_value 再 get_size
				
				//传出
				data.assign(_data, _data + data_size);
			}

			//断言：指定位置的值
			template<typename T>
			void get_this_value(_In_ const T & expected_value, _In_ const uint64_t column_index) const
			{
				T value{};
				get_value(value, column_index);
				utils_assert(value == expected_value);
			}

		private:
			friend class utils::SqliteReader;
			OneRowOfResult(sqlite3_stmt* stmt)
				: m_stmt{stmt}
			{
				m_column_n = sqlite3_column_count(m_stmt);
			}

			sqlite3_stmt* m_stmt{};
			uint64_t m_column_n{};	//列数
		};

		//打开文件
		SqliteReader(const std::string & filename)
		{
			//转为uft8格式
			std::string _filename;
			utils::character_code_function::convert_ansi_to_utf8(_filename, filename);

			//简单检查
			utils_assert(_filename.empty() == false && _filename[0] != ':');		//：开头的文件对sqlite有特别含义

			//打开文件
			auto err = sqlite3_open_v2(_filename.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr);
			utils_assert(err == SQLITE_OK);
		}
		~SqliteReader()
		{
			sqlite3_close(m_db);	//todo 检查返回值,同时要处理在析构函数中抛出异常的问题
		}
		//执行一个sql语句
		//sql: 编码: ANSI
		void query(_In_ const std::string & sql, _In_ std::function<void (_Out_ const OneRowOfResult & row)> meet_one_row)
		{
			//创建语句
			sqlite3_stmt* stmt{};
			utils::CallWhenReturn stmt_clean{[&]()
			{
				sqlite3_finalize(stmt);	//todo 检查返回值,同时要处理在析构函数中抛出异常的问题
			}};
			{
				std::string _sql;
				utils::character_code_function::convert_ansi_to_utf8(_sql, sql);

				const char* tail{};
				utils_assert(_sql.length() + 1 < std::numeric_limits<int>::max());
				auto err = sqlite3_prepare_v2(m_db, _sql.c_str(), (int)(_sql.length() + 1), &stmt, &tail);
				utils_assert(err == SQLITE_OK);
				utils_assert(tail == _sql.c_str() + _sql.length());
			}

			//执行
			while (1)
			{
				auto err = sqlite3_step(stmt);
				utils_assert(err == SQLITE_DONE || err == SQLITE_ROW);

				if (err == SQLITE_DONE)
					break;

				meet_one_row(OneRowOfResult{stmt});
			}
		}
		//查询，并且结果应该只有一行
		void query_with_one_and_only_one_result(_In_ const std::string & sql, _In_ std::function<void (_Out_ const OneRowOfResult & row)> meet_one_row)
		{
			uint64_t count{};
			query(sql, [&](_Out_ const OneRowOfResult & row)
			{
				utils_assert(count == 0);
				count++;

				meet_one_row(row);
			});
			utils_assert(count == 1);
		}
	private:
		sqlite3* m_db{};
	};
}


#include "utils/compiler/windows_header/maxmin__restore.inc"
