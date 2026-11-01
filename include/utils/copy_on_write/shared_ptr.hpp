#pragma once

#include <memory>
#include "utils/assert.hpp"

namespace utils::copy_on_write
{
	namespace inner::shared_ptr
	{
		struct s_one_object
		{
		public:
			using function_ptr_t_clone = void*(*)(const void* p);
			using function_ptr_t_delete = void(*)(const void* p);

		public:
			void* p{};
			uint64_t ref_count{};
			function_ptr_t_clone clone{};
			function_ptr_t_delete _delete{};
		};
	}


	//todo: 更新这个注释
	//写时拷贝的智能指针
	//note: 由于clone()的需要,T及其父类需要是单继承的
	//note: 不是多线程下多线程安全的. 多线程下单线程安全
	//note:
	//	使用时,需要T中涉及到的类型只能是:
	//		1.语言自带类型
	//		2.类似std::string这样虽然有动态内存分配,但普通拷贝时会分配新的内存的.
	//		3.类似std::vector这样的容器,理由同上
	//		4.写时拷贝的智能指针
	//	作为对照,列出一些不可以的:
	//		1.原生的指针类型  e.g. std::string*
	//		2.std::shared_ptr
	//		3.std::unique_ptr
	//example:
	//	struct s_0
	//	{
	//		int a{};
	//		std::vector<utils::copy_on_write::shared_ptr<s_big_struct>> b;
	//	
	//		virtual s_0* clone()
	//		{
	//			return new s_0{};
	//		}
	//	};
	//	struct s_1 : s_0
	//	{
	//		int c{};
	//	
	//		virtual s_1* clone()
	//		{
	//			return new s_1{};
	//		}
	//	};
	//	auto p = utils::copy_on_write::make_shared<s_1>();
	//	auto _a = p->a;			//read
	//	auto _c = p.get()->c;	//read
	//	auto _obj = *p;			//read
	//	p.writable()->_a = 123;	//write
	template<typename T>
	class shared_ptr
	{
		//简单检查
//		static_assert(std::is_trivially_copyable_v<T> && std::is_class_v<T>);
		static_assert(std::is_class_v<T>);	//todo: 看看能不能加些 普通= 的断言

#pragma region 友元
	private:
		//友元
		template<typename A>
		friend class utils::copy_on_write::shared_ptr;
		//友元
		template<typename A, typename B>
		friend shared_ptr<A> dynamic_pointer_cast(const shared_ptr<B> & sp);
		//友元
		template<typename A>
		friend shared_ptr<A> make_shared();
#pragma endregion

	private:
		using function_ptr_t_clone = inner::shared_ptr::s_one_object::function_ptr_t_clone;
		using function_ptr_t_delete = inner::shared_ptr::s_one_object::function_ptr_t_delete;

	private:
		utils::copy_on_write::inner::shared_ptr::s_one_object* m_object{};

		//构造函数
	public:
		shared_ptr()
		{
		}
#if 0
		
		//子类
		template<typename other_T, typename = std::enable_if_t<
			std::is_convertible_v<other_T*, T*> &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		shared_ptr(const shared_ptr<other_T> & o)
		{
			m_object = o.m_object;
			if (m_object)
				m_object->ref_count++;
		}
		//子类
		template<typename other_T, typename = std::enable_if_t<
			std::is_convertible_v<other_T*, T*> &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		shared_ptr(shared_ptr<other_T> && o)
		{
			m_object = o.m_object;
			o.m_object = nullptr;
		}
#endif
#if 0
		//父类/子类
		template<typename other_T, typename = std::enable_if_t<
			(std::is_convertible_v<other_T*, T*> || std::is_convertible_v<T*, other_T*>) &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		explicit shared_ptr(const shared_ptr<other_T> & o)
		{
			m_object = o.m_object;
			if (m_object)
				m_object->ref_count++;
		}
		//父类/子类
		template<typename other_T, typename = std::enable_if_t<
			(std::is_convertible_v<other_T*, T*> || std::is_convertible_v<T*, other_T*>) &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		explicit shared_ptr(shared_ptr<other_T> && o)
		{
			m_object = o.m_object;
			o.m_object = nullptr;
		}
#endif

#if 1
		//子类
		template<typename other_T, typename = std::enable_if_t<
			(std::is_convertible_v<other_T*, T*>) &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		shared_ptr(const shared_ptr<other_T> & o)
		{
			//简单检查
			utils_assert(o.get()->template is_conv_to<T>());

			m_object = o.m_object;
			if (m_object)
				m_object->ref_count++;
		}
		//子类
		template<typename other_T, typename = std::enable_if_t<
			(std::is_convertible_v<other_T*, T*>) &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		shared_ptr(shared_ptr<other_T> && o)
		{
			//简单检查
			utils_assert(o.get()->template is_conv_to<T>());

			m_object = o.m_object;
			o.m_object = nullptr;
		}
		//父类
		template<typename other_T, typename unused = other_T, typename = std::enable_if_t<
			(std::is_convertible_v<T*, other_T*>) &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		explicit shared_ptr(const shared_ptr<other_T> & o)
		{
			//简单检查
			utils_assert(o.get()->template is_conv_to<T>());

			m_object = o.m_object;
			if (m_object)
				m_object->ref_count++;
		}
		//父类
		template<typename other_T, typename unused = other_T, typename = std::enable_if_t<
			(std::is_convertible_v<T*, other_T*>) &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		explicit shared_ptr(shared_ptr<other_T> && o)
		{
			//简单检查
			utils_assert(o.get()->template is_conv_to<T>());

			m_object = o.m_object;
			o.m_object = nullptr;
		}
#endif

		//自身
		shared_ptr(const shared_ptr & o)
		{
			m_object = o.m_object;
			if (m_object)
				m_object->ref_count++;
		}
		//自身
		shared_ptr(shared_ptr && o)
		{
			m_object = o.m_object;
			o.m_object = nullptr;
		}

		//析构函数
	public:
		~shared_ptr()
		{
			if (m_object)
			{
				m_object->ref_count--;
				if (m_object->ref_count == 0)
				{
					m_object->_delete(m_object->p);
					delete m_object;
				}
			}
		}

		//operator =
	public:
		//子类
		template<typename other_T, typename = std::enable_if_t<
			std::is_convertible_v<other_T*, T*> &&
			std::is_class_v<other_T>
			>>
		shared_ptr & operator =(const shared_ptr<other_T> & o)
		{
			//简单检查
			utils_assert(o.get()->template is_conv_to<T>());

			m_object = o.m_object;
			if (m_object)
			{
				m_object->ref_count++;
			}
			return *this;
		}
		//子类
		template<typename other_T, typename = std::enable_if_t<
			std::is_convertible_v<other_T*, T*> &&
			std::is_class_v<other_T>
			>>
		shared_ptr & operator =(shared_ptr<other_T> && o)
		{
			//简单检查
			utils_assert(o.get()->template is_conv_to<T>());

			m_object = o.m_object;
			o.m_object = nullptr;
			return *this;
		}

#if 0
		//父类
		template<typename other_T, typename = std::enable_if_t<
			std::is_convertible_v<T*, other_T*> &&
			std::is_class_v<other_T>
			>>
		shared_ptr & operator =(const shared_ptr<other_T> & o)
		{
			m_object = o.m_object;
			if (m_object)
			{
				m_object->ref_count++;
			}
			return *this;
		}
		//父类
		template<typename other_T, typename = std::enable_if_t<
			std::is_convertible_v<T*, other_T*> &&
			std::is_class_v<other_T>
			>>
		shared_ptr & operator =(shared_ptr<other_T> && o)
		{
			m_object = o.m_object;
			o.m_object = nullptr;
			return *this;
		}
#endif

		//自身
		shared_ptr & operator =(const shared_ptr & o)
		{
			m_object = o.m_object;
			if (m_object)
			{
				m_object->ref_count++;
			}
			return *this;
		}
		//自身
		shared_ptr & operator =(shared_ptr && o)
		{
			m_object = o.m_object;
			o.m_object = nullptr;
			return *this;
		}

#if 0
		//转为子类
	public:
		template<typename other_T, typename = std::enable_if_t<
			std::is_convertible_v<other_T*, T*> &&
			std::is_class_v<other_T> &&
			std::is_same_v<T, other_T> == false
			>>
		auto to_child() const -> shared_ptr<other_T>
		{
			shared_ptr<other_T> re;
			re.m_object = m_object;
			if (re.m_object)
			{
				re.m_object->ref_count++;
			}
			return re;
		}
#endif

	public:
		//获取一个只读的指针
		template<typename other_T = T, typename = std::enable_if_t<
			(std::is_convertible_v<T*, other_T*> ||
			std::is_convertible_v<other_T*, T*>) &&
			std::is_class_v<other_T>
			>>
		const other_T* get() const
		{
			//简单检查
			utils_assert(m_object);

			return (other_T*)(m_object->p);
		}
		//获取一个只读的指针
		const T* operator->() const
		{
			return get();
		}
		//获得一个只读的引用
		const T & operator*() const
		{
			return *get();
		}
		//获取一个可写的指针
		template<typename other_T = T, typename = std::enable_if_t<
			(std::is_convertible_v<T*, other_T*> ||
			std::is_convertible_v<other_T*, T*>) &&
			std::is_class_v<other_T>
			>>
		other_T* writable()
		{
			//简单检查
			utils_assert(m_object);
			utils_assert(m_object->ref_count >= 1);
			utils_assert(m_object->clone);

			//复制
			if (m_object->ref_count > 1)
			{
				auto old_object = m_object;

				//不再引用之前的对象
				m_object->ref_count --;

				//简单检查
				utils_assert(m_object->ref_count > 0);

				m_object = nullptr;

				set(
					old_object->clone(old_object->p),
					old_object->clone,
					old_object->_delete);
			}
			
			return (other_T*)(const_cast<T*>(get()));
		}
		//转为bool
		explicit operator bool() const
		{
			return m_object;
		}

	private:
		//设置指针
		void set(void* p, const function_ptr_t_clone clone, const function_ptr_t_delete _delete)
		{
			//简单检查
			utils_assert(m_object == nullptr);

			//简单检查
			utils_assert(((const T*)p)->template is_conv_to<T>());

			auto new_object = new inner::shared_ptr::s_one_object{};
			new_object->p = p;
			new_object->ref_count = 1;
			new_object->clone = clone;
			new_object->_delete = _delete;

			m_object = new_object;
		}
#if 0
		//动态转换
		template<typename other_T>
		auto _dynamic_pointer_cast() const -> utils::copy_on_write::shared_ptr<other_T, real_T>
		{
			utils::copy_on_write::shared_ptr<other_T, real_T> re;

			if (m_object && dynamic_cast<other_T*>(m_object->p))
				return *this;
			else
				return {};
		}
#endif
	};

	namespace inner::shared_ptr
	{
		template<typename T>
		auto _clone(const T* p) -> T*
		{
			auto new_p = new T{};
			*new_p = *p;
			return new_p;
		}
		template<typename T>
		auto _delete(const T* p) -> void
		{
			delete p;
		}
	}

	//类似于 std::make_shared
	template<typename T>
	auto make_shared() -> utils::copy_on_write::shared_ptr<T>
	{
		using function_ptr_t_clone = utils::copy_on_write::shared_ptr<T>::function_ptr_t_clone;
		using function_ptr_t_delete = utils::copy_on_write::shared_ptr<T>::function_ptr_t_delete;

		utils::copy_on_write::shared_ptr<T> re;
		re.set(
			new T{},
			(function_ptr_t_clone)&inner::shared_ptr::_clone<T>,
			(function_ptr_t_delete)&inner::shared_ptr::_delete<T>);
		return re;
	}

//	//类似于 std::dynamic_pointer_cast
//	template<typename to_T, typename from_T, typename = std::enable_if_t<
//			(std::is_convertible_v<to_T*, from_T*> ||
//			std::is_convertible_v<from_T*, to_T*>) &&
//			std::is_class_v<to_T> &&
//			std::is_class_v<from_T>
//			>>
//	auto dynamic_pointer_cast(const utils::copy_on_write::shared_ptr<from_T> & sp) ->
//		utils::copy_on_write::shared_ptr<to_T>
//	{
////		return sp._dynamic_pointer_cast<to_T>();
//		return sp;
//	}
}