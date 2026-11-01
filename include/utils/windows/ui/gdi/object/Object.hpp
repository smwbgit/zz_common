#pragma once

#include <windows.h>
#include <utils/assert.hpp>

namespace utils::windows::ui::gdi::object
{
	//选择对象
	inline HGDIOBJ select_object(HDC hdc, HGDIOBJ h)
	{
		auto old = ::SelectObject(hdc, h);
		utils_assert(old);
		return old;
	}
	//删除对象
	inline void delete_object(HGDIOBJ h)
	{
		utils_assert(::DeleteObject(h));
	}

	//一个gdi对象
	template<typename T>
	class Object
	{
	private:
		std::optional<T> m_object;
		std::optional<T> m_old_object;
	public:
		Object() = default;
		explicit Object(const T hObject)
			: m_object{ hObject }
		{
		}
		// 禁止复制
		Object(const Object &) = delete;
		// 禁止复制
		Object & operator =(const Object &) = delete;
		// 移动
		Object(Object && o)
		{
			// 复制对方
			m_object = o.m_object;
			m_old_object = o.m_old_object;

			// 清空对方
			o.m_object.reset();
			o.m_old_object.reset();
		}
		// 移动
		Object & operator =(Object && o)
		{
			// 释放自身
			reset();

			// 复制对方
			m_object = o.m_object;
			m_old_object = o.m_old_object;

			// 清空对方
			o.m_object.reset();
			o.m_old_object.reset();

			// 完成
			return *this;
		}
		~Object()
		{
			// 删除对象
			reset();
		}
	public:
		operator T() const
		{
			// 简单检查
			utils_assert(m_object);

			// 给出
			return *m_object;
		}
	public:
		//选入自己
		void select_me(const HDC hdc)
		{
			//简单检查
			utils_assert(!m_old_object);
			utils_assert(m_object);

			//选入自己
			m_old_object = (T)gdi::object::select_object(hdc, *m_object);
		}
		//选入旧值
		void select_old(const HDC hdc)
		{
			//简单检查
			utils_assert(m_old_object);
			utils_assert(m_object);

			//选入旧值
			const auto old_object = (T)gdi::object::select_object(hdc, *m_old_object);

			//更新状态
			m_old_object.reset();
			
			//简单检查
			utils_assert(old_object == *m_object);
		}
		// 删除对象
		void reset()
		{
			// 简单检查
			utils_assert(!m_old_object);

			// 删除对象
			if (m_object)
			{
				gdi::object::delete_object(*m_object);
			}
		}
	};
}