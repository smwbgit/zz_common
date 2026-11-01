#pragma once


namespace utils
{
	//模拟std::optional，为不支持c++17版本的编译器提供个类似的功能
	template<typename T>
	class optional
	{
	public:
#pragma region 构造函数和赋值函数
		optional()
			: m_has_value{false}
		{
		}
		optional(const T & v)
			: m_value{v}
			, m_has_value{true}
		{
		}
		optional(T && v)
			: m_value{std::move(v)}
			, m_has_value{true}
		{
		}
		template<typename... Args>
		optional(Args &&... args)
			: m_value{std::forward<Args>(args)...}
			, m_has_value{true}
		{
		}

		optional(const optional<T> & o)
			: m_value{o.m_value}
			, m_has_value{o.m_has_value}
		{
		}
		optional(optional<T> && o)
			: m_value{std::move(o.m_value)}
			, m_has_value{o.m_has_value}
		{
		}
			
		optional<T> & operator = (const T & v)
		{
			m_value = v;
			m_has_value = true;
			return *this;
		}
		optional<T> & operator = (T && v)
		{
			m_value = std::move(v);
			m_has_value = true;
			return *this;
		}

		optional<T> & operator = (const optional<T> & o)
		{
			m_value = o.m_value;
			m_has_value = o.m_has_value;
			return *this;
		}
		optional<T> & operator = (optional<T> && o)
		{
			m_value = std::move(o.m_value);
			m_has_value = o.m_has_value;
			return *this;
		}
		//创建一个值，并返回引用。如果已经有值，出错
		T & create_value()
		{
			utils_assert(false == has_value());
			*this = T{};
			return value();
		}
#pragma endregion


#pragma region 比较函数
		bool operator ==(const optional<T> & o) const
		{
			if (has_value() != o.has_value())
				return false;

			if (has_value())
			{
				return m_value == o.m_value;
			}
			return true;
		}
#pragma endregion

		//是否有值
		bool has_value() const
		{
			return m_has_value;
		}
		//获得值
		const T & value() const
		{
			return m_value;
		}
		//获得值
		T & value()
		{
			return m_value;
		}
		//清空
		void reset()
		{
			m_has_value = false;
		}

	private:
		bool m_has_value{};
		T m_value{};
	};


	//引用类型的特化
	template<typename T>
	class optional<T&>
	{
	public:
#pragma region 构造函数和赋值函数
		optional()
			: m_has_value{false}
		{
		}
		optional(T & v)
			: m_pointer{&v}
			, m_has_value{true}
		{
		}
		optional(T && v) = delete;

		optional(const optional<T&> & o)
			: m_pointer{o.m_pointer}
			, m_has_value{o.m_has_value}
		{
		}
		optional(optional<T&> && o)
			: m_pointer{o.m_pointer}
			, m_has_value{o.m_has_value}
		{
		}
			
		optional<T&> & operator = (T & v)
		{
			m_pointer = &v;
			m_has_value = true;
			return *this;
		}
		optional<T&> & operator = (T && v) = delete;

		optional<T&> & operator = (const optional<T&> & o)
		{
			m_pointer = o.m_pointer;
			m_has_value = o.m_has_value;
			return *this;
		}
		optional<T&> & operator = (optional<T&> && o)
		{
			m_pointer = o.m_pointer;
			m_has_value = o.m_has_value;
			return *this;
		}
#pragma endregion


#pragma region 比较函数
		bool operator ==(const optional<T&> & o) const
		{
			if (has_value() != o.has_value())
				return false;

			if (has_value())
			{
				return m_pointer == o.m_pointer;
			}
			return true;
		}
#pragma endregion

		//是否有值
		bool has_value() const
		{
			return m_has_value;
		}
		//获得值
		T & value() const
		{
			return *m_pointer;
		}

	private:
		bool m_has_value{};
		T* m_pointer{};
	};
}
