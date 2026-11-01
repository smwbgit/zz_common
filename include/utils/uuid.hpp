#pragma once

#include <utils/compiler/windows_header/maxmin__eliminate.inc>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <utils/compiler/windows_header/maxmin__restore.inc>

namespace utils
{
#if 0
	class uuid
	{
	public:
		uuid()
		{
			//设置初值
			memset(&m_uuid, 0, sizeof(m_uuid));
		}
		uuid(const boost::uuids::uuid & _uuid)
			: m_uuid{_uuid}
		{
		}
		uuid(boost::uuids::uuid && _uuid)
			: m_uuid{std::move(_uuid)}
		{
		}
	private:
		boost::uuids::uuid m_uuid;
	};
#endif
#if 0
	class uuid : public boost::uuids::uuid
	{
	public:
		using boost::uuids::uuid::uuid;
		uuid()
		{
			//设置初值
			memset(data, 0, sizeof(data));
		}
	};
#endif
	using uuid = boost::uuids::uuid;
}
namespace utils::uuid_function
{
	//获得一个uuid
	inline utils::uuid new_uuid()
	{
		return boost::uuids::random_generator()();
	}
	//获得一个uuid字符串，带'-'
	//格式： hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh where h is hexadecimal
	inline void get_string_with_dash(std::string & uuid)
	{
		uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	}
}