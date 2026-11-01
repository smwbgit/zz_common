#pragma once

#include <utils/data.hpp>
#include <utils/text/hex.hpp>
#include <utils/random.hpp>

namespace utils
{
	//代表一个固定大小的数据
	/*
		示例代码：

		//代表一个20字节的id
		class Id
		{
		public:
			utils::SizedData<20> m_data;

		public:
			//构造函数
			explicit Id(utils::data_view data)
				: m_data{data}
			{
			}
			//构造函数
			Id() = default;
			Id(const Id &) = default;
			Id(Id &&) = default;
		public:
			//赋值函数
			Id & operator = (utils::data_view data)
			{
				m_data = data;
				return *this;
			}
			//赋值函数
			Id & operator = (const Id &) = default;
			Id & operator = (Id &&) = default;
		public:
			//比较运算
			auto operator<=>(const Id &) const = default;
			//序列化
			template<bool is_saving, typename serializer_T>
			void serialize_support(serializer_T & file)
			{
				file & m_data;
			}
		};
	*/
	template<utils::u8 size_T>
	class SizedData
	{
	private:
		std::array<u1, size_T> m_data{};
	public:
		//构造函数
		explicit SizedData(utils::data_view data)
		{
			//简单检查
			utils_assert(data.size() == size());

			//保存
			memcpy(m_data.data(), data.addr(), data.size());
		}
		//构造函数
		SizedData() = default;
		SizedData(const SizedData &) = default;
		SizedData(SizedData &&) = default;
	public:
		//赋值函数
		SizedData & operator = (utils::data_view data)
		{
			*this = SizedData{data};
			return *this;
		}
		//赋值函数
		SizedData & operator = (const SizedData &) = default;
		SizedData & operator = (SizedData &&) = default;
#pragma region 基本功能
	public:
		//获取地址
		u1* addr()
		{
			return &m_data[0];
		}
		//特化
		const u1* addr() const
		{
			return &m_data[0];
		}
		//获取大小
		static constexpr u8 size()
		{
			return size_T;
		}
		//获取子数据
		utils::data sub_data(u8 offset, u8 size) const
		{
			//准备数据
			auto data_view = utils::data_view{m_data};

			//获取
			const auto data = data_view.sub_data(offset, size);

			//给出
			return data;
		}
		//遍历
		auto begin()
		{
			return m_data.begin();
		}
		//特化
		auto begin() const
		{
			return m_data.begin();
		}
		//遍历
		auto end()
		{
			return m_data.end();
		}
		//特化
		auto end() const
		{
			return m_data.end();
		}
		//访问操作
		u1 & operator[](u8 i)
		{
			return m_data[i];
		}
		//特化
		const u1 & operator[](u8 i) const
		{
			return m_data[i];
		}
		//转为字符串，十六进制表示
		std::u8string to_text_hex() const
		{
			return utils::text::hex::to_text(m_data, false);
		}
		//转为data_view
		operator utils::data_view() const
		{
			return {m_data};
		}
		//获得底层表示
		std::array<u1, size_T> & get_native()
		{
			return m_data;
		}
		//特化
		const std::array<u1, size_T> & get_native() const
		{
			return m_data;
		}
		//比较运算
		auto operator<=>(const SizedData<size_T> &) const = default;
		//序列化
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & m_data;
		}
#pragma endregion
#pragma region 赋值
	public:
		//随机生成
		void assign_by_random()
		{
			//生成随机数据
			auto data = utils::random::make_random(size());

			//保存
			*this = SizedData{std::move(data)};
		}
		//使用文本生成
		void assign_by_text_hex(const std::u8string & text)
		{
			//进行转换
			auto data = utils::text::hex::from_text(text, false);

			//简单检查
			utils_assert(data.size() == size());

			//保存
			*this = SizedData{std::move(data)};
		}
#pragma endregion
	};

	//简便函数：生成
	namespace make
	{
		//简便函数：使用数据生成
		template<typename T>
		inline T SizedData(utils::data_view data)
		{
			T value;
			value.m_data = data;
			return value;
		}
	}
}