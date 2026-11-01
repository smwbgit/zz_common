#pragma once

#include <vector>
#include <string>
#include <utils/assert.hpp>
#include <utils/concepts.hpp>
#include <utils/integer_type.hpp>
#include <utils/basic/store_this_integer.hpp>

namespace utils
{
	class data_view;	//向前声明

	class data
	{
	public:
		static const u8 npos = (u8)-1;
	public:
		std::vector<u1> m_data;
	public:
#pragma region 构造函数
		data()
		{
		}
		data(const data & other)
			: m_data{other.m_data}
		{
		}
		data(data && other)
			: m_data{std::move(other.m_data)}
		{
		}
		explicit data(const std::vector<u1> & data)
			: m_data{data}
		{
		}
		explicit data(std::vector<u1> && data)
			: m_data{std::move(data)}
		{
		}
		//note: 不复制末尾0
		explicit data(const std::string & data)
		{
			m_data.assign(data.begin(), data.end());
		}
		//note: 不复制末尾0
		explicit data(const std::wstring & data)
		{
			m_data.assign((const char*)data.c_str(), (const char*)(data.c_str() + data.size()));
		}
		//note: 不复制末尾0
		explicit data(const std::u8string & data)
		{
			m_data.assign((const char*)data.c_str(), (const char*)(data.c_str() + data.size()));
		}
		//延迟定义
		explicit data(utils::data_view other);
#pragma endregion
#pragma region 赋值函数
		data & operator =(const data & other)
		{
			m_data = other.m_data;
			return *this;
		}
		data & operator =(data && other)
		{
			m_data = std::move(other.m_data);
			return *this;
		}
		//note: 不复制结尾0
		data & operator =(const std::string & data)
		{
			m_data.assign(data.begin(), data.end());
			return *this;
		}
		//note: 不复制结尾0
		data & operator =(const std::wstring & data)
		{
			m_data.assign((const char*)data.c_str(), (const char*)(data.c_str() + data.size()));
			return *this;
		}
		void assign(_In_ const void* data, _In_ const u8 size)
		{
			m_data.assign((const u1*)data, (const u1*)data + size);
		}
#pragma endregion
#pragma region 转换函数
		//把m_data给出
		std::vector<u1> & wrapped_type()
		{
			return m_data;
		}
		//特化
		const std::vector<u1> & wrapped_type() const
		{
			return m_data;
		}
#pragma endregion
#pragma region 查找和替换
		//查找一段二进制数据
		//return: 如果找到，返回所在位置，否则返回npos
		u8 find(const std::string & data) const
		{
			utils_assert(data.empty() == false);

			for (u8 i = 0; i < size()-data.size()+1; i++)
			{
				if(memcmp(addr()+i, data.c_str(), data.size()) == 0)
					return i;
			}
			return npos;
		}
		//替换一个唯一存在的数据
		void replace_unique(const utils::data & from, const utils::data & to)
		{
			//简单检查
			utils_assert(!from.empty());

			auto pos = [this,&from]()
			{
				bool is_meeted{};
				u8 pos{};
				for(u8 i = 0; i < this->size(); i++)
				{
					if(i + from.size() > this->size()) break;

					if(memcmp(this->addr()+i,from.addr(),from.size()) == 0)
					{
						//简单检查
						utils_assert(false == is_meeted);

						is_meeted = true; pos = i;
					}
				}

				//简单检查
				utils_assert(is_meeted);

				return pos;
			}();

			if(from.size() == to.size())
			{
				memmove(this->addr()+pos, to.addr(),to.size());
			}
			else
			{
				utils::data _new;
				_new.append(this->addr(), pos);
				_new.append(to.addr(),to.size());
				_new.append(this->addr()+pos+from.size(), this->size()-(pos+from.size()));
				*this = std::move(_new);
			}
		}
		//特化
		void replace_unique(u8 from, u8 to)
		{
			utils::data _from; _from.append((u1*)&from, 8);
			utils::data _to__; _to__.append((u1*)&to, 8);
			return replace_unique(_from,_to__);
		}
#pragma endregion
#pragma region 截取
		//截取
		utils::data subdata(_In_ u8 offset, _In_ u8 size) const
		{
			//简单检查
			utils_assert(offset + size <= this->size());

			utils::data _data;
			_data.assign(addr() + offset, size);
			return _data;
		}
		//特化
		void subdata(_Out_ void* data, _In_ u8 offset, _In_ u8 size) const
		{
			//简单检查
			utils_assert(offset + size <= this->size());

			memmove(data, addr() + offset, size);
		}
#pragma endregion
#pragma region 比较
		bool operator == (const data &) const = default;
#pragma endregion
#pragma region 拼接
	public:
		utils::data & operator +=(_In_ utils::data_view data);	//在utils::data_view后实现
		utils::data operator +(_In_ utils::data_view data);		//在utils::data_view后实现
		//在最后添加数据
		void append(_In_ const utils::data_view data);	//在utils::data_view后实现
		//特化
		//size: 要添加的大小
		void append(_In_ const utils::data & data, _In_ const u8 size)
		{
			//简单检查
			utils_assert(size <= data.size());

			//保存
			m_data.insert(m_data.end(), data.addr(), data.addr()+size);
		}
		//特化
		void append(_In_ u1 byte)
		{
			m_data.push_back(byte);
		}
		//特化
		void append(_In_ const u1* data, _In_ const u8 size)
		{
			m_data.insert(m_data.end(), data, data+size);
		}
#pragma endregion
	public:
		u1 & operator[](u8 index)
		{
			return m_data[index];
		}
		const u1 & operator[](u8 index) const
		{
			return m_data[index];
		}
	public:
		const u1* addr() const
		{
			return m_data.data();
		}
		u1* addr()
		{
			return m_data.data();
		}
		u8 size() const
		{
			return m_data.size();
		}
		bool empty() const
		{
			return size() == 0;
		}
		void reserve(_In_ u8 size)
		{
			m_data.reserve(size);
		}
		void clear()
		{
			m_data.clear();
		}
		void resize(_In_ u8 size)
		{
			m_data.resize(size);
		}
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & m_data;
		}
	};
}

namespace utils
{
	//一个不能修改的,连续的内存区
	class data_view
	{
	private:
		const u1* m_addr{};
		u8 m_size{};
#pragma region 构造函数
	public:
		data_view()
		{
		}
		data_view(_In_ const void* addr, _In_ const u8 size)
			: m_addr{(const u1*)addr}
			, m_size{size}
		{
		}
		data_view(_In_ const void* addr, _In_ const utils::concepts::integer auto size)
			: m_addr{(const u1*)addr}
			, m_size{utils::basic::store_in<u8>(size)}
		{
		}
		data_view(const std::vector<u1> & data)
			: m_addr{data.data()}
			, m_size{data.size()}
		{
		}
		template<size_t size>
		data_view(const std::array<u1, size> & data)
			: m_addr{data.data()}
			, m_size{data.size()}
		{
		}
		explicit data_view(const std::string & other)
			: m_addr{(const u1*)other.c_str()}
			, m_size{other.length()}
		{
		}
		explicit data_view(const std::u8string & other)
			: m_addr{(const u1*)other.c_str()}
			, m_size{other.length()}
		{
		}
		data_view(const utils::data & data)
			: m_addr{data.addr()}
			, m_size{data.size()}
		{
		}
		//特化
		//data_view(utils::data &&) = delete;	//note: 此条似乎不用加。因为，不让data_view引用到，要消失的数据，是用户的责任。而，有时，是需要做为一个右值引用，被传入的。比如，仅在某条语句中，被使用时。
		data_view(const data_view & other)
			: m_addr{other.m_addr}
			, m_size{other.m_size}
		{
		}
#pragma endregion
#pragma region 赋值函数
	public:
		data_view & operator =(const std::vector<u1> & data)
		{
			m_addr = data.data();
			m_size = data.size();
			return *this;
		}
		data_view & operator =(const data_view & other)
		{
			m_addr = other.m_addr;
			m_size = other.m_size;
			return *this;
		}
		data_view & operator =(const utils::data & data)
		{
			m_addr = data.addr();
			m_size = data.size();
			return *this;
		}
#pragma endregion
#pragma region 操作符
		const u1 & operator[](u8 index) const
		{
			//简单检查
			utils_assert(index < size());

			return addr()[index];
		}
#pragma endregion
#pragma region 遍历
		struct iterator
		{
		private:
			const u1* m_p{};
		public:
			iterator(const utils::data_view* _this, u8 index)
				: m_p{_this->addr() + index}
			{
			}
		public:
			u1 operator *() const
			{
				return *m_p;
			}
			void operator++()
			{
				m_p++;
			}
			std::strong_ordering operator <=>(const iterator & o) const = default;
		};

		iterator begin()
		{
			return iterator{this, 0};
		}
		iterator end()
		{
			return iterator{this, size()};
		}
#pragma endregion
#pragma region 转换为其他类型
	public:
		//note: 转换后std::string.size() == m_size
		std::string string() const
		{
			std::string re;
			re.assign((const char*)m_addr, m_size);
			return re;
		}
		//note: 转换后std::string.size() == m_size
		std::wstring wstring() const
		{
			//简单检查
			utils_assert(m_size % 2 == 0);

			std::wstring re;
			re.assign((const wchar_t*)m_addr, m_size / 2);
			return re;
		}
		//获取一份复制的数据
		utils::data get_data() const
		{
			utils::data data;
			data.assign(m_addr, m_size);
			return data;
		}
#pragma endregion
	public:
		//比较二进制的内容是否相同
		//note: 进行'深'比较
		bool operator ==(const data_view & other) const
		{
			if (size() != other.size())
				return false;

			if (0 == memcmp(addr(), other.addr(), size()))
				return true;
			else
				return false;
		}
	public:
		//获得数据地址
		const u1* addr() const
		{
			return m_addr;
		}
		//获得数据大小
		u8 size() const
		{
			return m_size;
		}
		//是否为空
		bool empty() const
		{
			return m_size == 0;
		}
		//获得子数据
		//note: 需要满足: 或者size为0, 或者[offset, offset+size)在数据范围内
		//offset: 子数据的起始位置
		//size: 子数据的大小
		utils::data_view sub_view(_In_ const u8 offset, _In_ const u8 size) const
		{
			//给出空数据
			if (size == 0)
				return {};

			//简单检查
			utils_assert(offset < this->size());
			utils_assert(offset < offset + size);	//防止加法溢出
			utils_assert(offset + size <= this->size());

			//给出
			return {this->addr() + offset, size};
		}
		//特化: 返回: 从offset到数据结束
		utils::data_view sub_view(_In_ const u8 offset) const
		{
			//转发
			return sub_view(offset, size() - offset);
		}
		//特化
		void sub_view(_Out_ utils::data_view & data_view, _In_ const u8 offset, _In_ const u8 size) const
		{
			//转发
			data_view = sub_view(offset, size);
		}
		//特化
		void sub_view(_Out_ utils::data_view & data_view, _In_ const u8 offset) const
		{
			//转发
			data_view = sub_view(offset);
		}
		//特化: 获得子数据
		utils::data sub_data(_In_ const u8 offset, _In_ const u8 size) const
		{
			return sub_view(offset, size).get_data();
		}
		//特化
		utils::data sub_data(_In_ const u8 offset) const
		{
			return sub_view(offset).get_data();
		}
		//是否有指定前缀
		bool has_prefix(_In_ const utils::data_view prefix) const
		{
			if (size() < prefix.size())
				return false;

			auto _prefix = sub_view(0, prefix.size());

			return _prefix == prefix;
		}
		//去掉前缀
		void strip_prefix(_Out_ utils::data_view & striped, _In_ const utils::data_view prefix) const
		{
			//简单检查
			utils_assert(has_prefix(prefix));

			//给出
			sub_view(striped, prefix.size());
		}
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			//简单检查
			static_assert(is_saving);

			//todo 看看有没有什么办法，能不用复制，又能时刻和 utils::data 里的 serialize_support 保持同步
			file & utils::data{*this};
		}
	};
}

namespace utils
{
	//utils::data的延迟定义的函数

	inline data::data(utils::data_view other)
		: m_data{ other.addr(), other.addr() + other.size() }
	{
	}
	//见定义
	inline void data::append(_In_ const utils::data_view data)
	{
		if (data.empty())
			return;

		m_data.insert(m_data.end(), data.addr(), data.addr() + data.size());
	}
	inline utils::data & data::operator +=(_In_ utils::data_view data)
	{
		//转发
		append(data);

		return *this;
	}
	inline utils::data data::operator +(_In_ utils::data_view data)
	{
		//准备数据
		utils::data total = *this;

		//转发
		total += data;

		//给出
		return total;
	}
}


namespace utils
{
	//一个可写的，不能修改长度的内存区
	class data_view_writable : public data_view
	{
	public:
		using data_view::data_view;

		//重写
		u1* addr()
		{
			return (u1*)data_view::addr();
		}

		//重写
		u1 & operator[](u8 index)
		{
			//简单检查
			utils_assert(index < size());

			return addr()[index];
		}
	};
}