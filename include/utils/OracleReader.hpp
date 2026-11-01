#pragma once

#include <occi.h>
#include <functional>
#include "utils/assert.hpp"
#include "utils/on_leave_scope.hpp"
#include "utils/Date.hpp"

namespace utils
{
	//读取oracle数据库
	class OracleReader
	{
		//类型定义
	private:
		using _ResultSet = oracle::occi::ResultSet;
		using Blob = oracle::occi::Blob;
		using Environment = oracle::occi::Environment;
		using Connection = oracle::occi::Connection;
		using SQLException = oracle::occi::SQLException;
		using Statement = oracle::occi::Statement;
		
	public:
		//查询结果的一行
		class ResultSet
		{
		public:
#pragma region 获得某列的值
			//col_index: 哪一列
			template<typename T>
			void get_value(_Out_ T & value, _In_ unsigned int col_index)
			{
				static_assert(false, "");
			}
			template<>
			void get_value<std::string>(_Out_ std::string & value, _In_ unsigned int col_index)
			{
				try {
					value = m_rs->getString(col_index);
				}
				catch (SQLException &)
				{
					utils_throw();
				}
			}
			template<>
			void get_value<utils::Date>(_Out_ utils::Date & value, _In_ unsigned int col_index)
			{
				try {
					auto _date = m_rs->getDate(col_index);
					int y{};
					unsigned int m{}, d{}, _h{}, _m{}, _s{};
					_date.getDate(y, m, d, _h, _m, _s);
					value.set(y, m, d);
				}
				catch (SQLException &)
				{
					utils_throw();
				}
			}
			template<>
			void get_value<int>(_Out_ int & value, _In_ unsigned int col_index)
			{
				try {
					value = m_rs->getInt(col_index);
				}
				catch (SQLException &)
				{
					utils_throw();
				}
			}
			template<>
			void get_value<double>(_Out_ double & value, _In_ unsigned int col_index)
			{
				try {
					value = m_rs->getDouble(col_index);
				}
				catch (SQLException &)
				{
					utils_throw();
				}
			}
#pragma endregion
			//获取一个blob
			template <typename T>
			void read_blob(_Out_ std::vector<T> & data, _In_ unsigned int col_index)
			{
				try 
				{
					//打开blob
					data.clear();
					Blob blob = m_rs->getBlob(col_index);
					blob.open(oracle::occi::OCCI_LOB_READONLY);
					auto blob_size = blob.length();
					if (blob_size == 0)
						return;

					//简单检查
					utils_assert(blob_size % sizeof(T) == 0);

					//读取数据
					data.resize(blob_size / sizeof(T));
					unsigned int offset = 1;
					utils_assert(blob_size == blob.read(blob_size, (unsigned char*)&data[0], blob_size, offset));
					blob.close();
				}
				catch(SQLException &)
				{
					utils_throw();
				}
			}
		private:
			ResultSet(_ResultSet* rs) : m_rs{rs} {}

			friend class OracleReader;
			_ResultSet* m_rs{};
		};

	public:
		~OracleReader()
		{
			if (m_conn)
				m_env->terminateConnection(m_conn);
			if (m_env)
				Environment::terminateEnvironment(m_env);
		}
		void init(const std::string& server, const std::string& user, const std::string& passwd)
		{
			try {
				m_env = Environment::createEnvironment();
				utils_assert(m_env);

				m_conn = m_env->createConnection(user, passwd, server);
				utils_assert(m_conn);
			}
			catch (SQLException & e)
			{
				auto re = e.getErrorCode();
				m_conn = nullptr;
				utils_throw();
			}
		}
		//查询，返回每一行
		void query(const std::string & sql, std::function<void(ResultSet & rs)> meet_one_row)
		{
			try {
				Statement* stmt = m_conn->createStatement();
				utils_assert(stmt);
				utils_on_leave_scope(m_conn->terminateStatement(stmt));

				_ResultSet* rs = stmt->executeQuery(sql);
				while (rs->next())
					meet_one_row(ResultSet{rs});

				stmt->closeResultSet(rs);
			}
			catch (SQLException &)
			{
				utils_throw();
			}
		}
		//查询，并且结果应该只有一行
		void query_with_one_and_only_one_result(const std::string & sql, std::function<void(ResultSet & rs)> meet_one_row)
		{
			uint64_t count{};
			query(sql, [&](ResultSet & rs)
			{
				utils_assert(count == 0);
				count++;

				meet_one_row(rs);
			});
			utils_assert(count == 1);
		}
		//执行语句
		void execute(const std::string & sql)
		{
			try {
				Statement* stmt = m_conn->createStatement();
				utils_assert(stmt);
				utils_on_leave_scope(m_conn->terminateStatement(stmt));

				stmt->executeUpdate(sql);
			}
			catch (SQLException &)
			{
				utils_throw();
			}
		}

	private:
		Environment* m_env = nullptr;
		Connection* m_conn = nullptr;
	};
}