#pragma once

#include <vector>
#include <functional>

//note: 关于属性:
//			一个属性：可以保存一个值，并在它发生变更时，对外发出通知
//			这里的属性，都是“只对最新值感兴趣，可以跳过 次最新 值的感知”的属性
//note: 关于线程安全:
//			utils::value::property 命名空间里的类，都是非线程安全的
//			utils::value::property::thread_safe 命名空间里的类，都是线程安全的
namespace utils::value::property
{
	//属性
	template<typename T>
	class Property
	{
	private:
		T m_value{};
		std::vector<std::function<void()>> m_all_listener;
		std::vector<std::function<void(T & new_value)>> m_all_checker;
		std::vector<std::function<void(const T & new_value)>> m_all_assertor;
	public:
		Property()
		{
		}
		explicit Property(const T & value)
			: m_value{value}
		{
		}
		void set_value(T value)
		{
			//复制检查函数
			//note: 为了支持，检查过程中，添加了新的检查函数，把检查函数复制一份。下面的通知函数同理
			const auto all_checker = m_all_checker;

			//进行检查
			for(const auto & checker : all_checker)
			{
				checker(value);
			}

			//复制断言函数
			const auto all_assertor = m_all_assertor;

			//进行断言
			for(const auto & assertor : all_assertor)
			{
				assertor(value);
			}

			//已为此值
			if(m_value == value)
				return;

			//保存新值
			m_value = value;

			//复制通知函数
			const auto all_listener = m_all_listener;

			//发出通知
			for(const auto & listener : all_listener)
			{
				listener();		//这里特意不传值，让它们自己去查新值，来和线程安全版本，保持参数个数一致
			}
		}
		T get_value() const
		{
			return m_value;
		}
		void add_listener(std::function<void()> listener)
		{
			//保存
			m_all_listener.push_back(listener);
		}
		//特化
		Property & operator += (std::function<void()> listener)
		{
			//转发
			add_listener(listener);

			return *this;
		}
		//添加断言
		void add_assertor(std::function<void(const T & new_value)> assertor)
		{
			//保存
			m_all_assertor.push_back(assertor);
		}
		void add_checker(std::function<void(T & new_value)> checker)
		{
			//保存
			m_all_checker.push_back(checker);
		}
		void check_value()
		{
			//重新赋值一下，以便于运行检查函数
			set_value(m_value);
		}
		//添加一个绑定
		//note: 添加后，两个属性合二为一，更新一个，另一个也会跟着变，两个属性的所有检查
		//		函数和通知函数，都会被调用
		//note: 因为添加时，两个属性的值可能不一样，所以这里规定，other 变为 *this 里的值。
		void add_bind(Property & other)
		{
			//统一当前值
			other.set_value(m_value);

			//准备函数
			auto _add_listener = [](Property & a, Property & b)
			{
				//当a的值改变时
				a += [&a, &b]()
				{
					//获得a的当前值
					const auto value = a.get_value();

					//赋给b
					b.set_value(value);
				};
			};

			//相互添加通知函数
			_add_listener(*this, other);
			_add_listener(other, *this);
		}
	};
}