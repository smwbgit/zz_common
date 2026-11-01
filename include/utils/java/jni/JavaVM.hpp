#pragma once

#include <jni.h>
#include <utils/assert.hpp>
#include <utils/text/encoding.hpp>

namespace utils::java::jni
{
	namespace inner
	{
		//对 jobject/etc 的简单包装
		template <typename jobject_T>
		class jobject_basic
		{
		private:
			jobject_T m_value{};
			std::function<void()> m_on_clean;

		public:
			jobject_basic(jobject_T value, std::function<void()> on_clean)
				: m_value{value}
				, m_on_clean{on_clean}
			{
				//非空
				if (m_value)
				{
					//简单检查
					utils_assert(m_on_clean);
				}
			}
			~jobject_basic()
			{
				//非空
				if (m_value)
				{
					//清理
					m_on_clean();
				}
			}

			//禁止复制
		public:
			jobject_basic(const jobject_basic &) = delete;
			jobject_basic & operator=(const jobject_basic &) = delete;

			//可以移动
		public:
			jobject_basic(jobject_basic && other)
				: m_value{other.m_value}, m_on_clean{other.m_on_clean}
			{
				other.m_value = nullptr;
				other.m_on_clean = {};
			}
			jobject_basic & operator=(jobject_basic && other)
			{
				m_value = other.m_value;
				m_on_clean = other.m_on_clean;

				other.m_value = nullptr;
				other.m_on_clean = {};

				return *this;
			}

		public:
			// 获得底层表示
			[[nodiscard]] jobject_T get_native() const
			{
				return m_value;
			}
			// 转为 bool
			operator bool() const
			{
				return m_value;
			}
		};
	}

	// 对 同名类 的简单包装
	using jobject		= inner::jobject_basic<::jobject>;
	using jclass		= inner::jobject_basic<::jclass>;
	using jstring		= inner::jobject_basic<::jstring>;
	using jdoubleArray	= inner::jobject_basic<::jdoubleArray>;

	// 对 同名类 的简单包装
	class jmethodID
	{
	private:
		::jmethodID m_value{};

	public:
		jmethodID(::jmethodID value)
			: m_value{value}
		{
			//简单检查
			utils_assert(m_value);
		}
		~jmethodID()
		{
		}

	public:
		jmethodID(const jmethodID &) = default;
		jmethodID & operator=(const jmethodID &) = default;

	public:
		jmethodID(jmethodID &&) = default;
		jmethodID & operator=(jmethodID &&) = default;

	public:
		// 获得底层表示
		[[nodiscard]] ::jmethodID get_native() const
		{
			return m_value;
		}
	};

	// 对 同名类 的简单包装
	class JNIEnv
	{
	private:
		::JNIEnv* const m_env = nullptr;
		std::function<void()> m_on_clean;

	public:
		JNIEnv(::JNIEnv* env, std::function<void()> on_clean = [](){})
			: m_env{env}
			, m_on_clean{on_clean}
		{
			//简单检查
			utils_assert(m_env);
			utils_assert(m_on_clean);
		}
		~JNIEnv()
		{
			//清理
			m_on_clean();
		}

		//禁止复制
	public:
		JNIEnv(const JNIEnv &) = delete;
		JNIEnv & operator=(const JNIEnv &) = delete;

	public:
		// 获得底层表示
		[[nodiscard]] ::JNIEnv* get_native() const
		{
			return m_env;
		}
		// 对 同名函数 的简单包装
		[[nodiscard]] jstring NewStringUTF(const std::u8string & text) const
		{
			//准备数据
			const auto* text_2 = (const char*)text.c_str();

			//创建
			::jstring text_3 = m_env->NewStringUTF(text_2);

			//简单检查
			assert_ok(text_3);

			//准备函数：清理
			auto on_clean = [this, text_3]()
			{
				//清理
				m_env->DeleteLocalRef(text_3);
			};

			//给出
			return {text_3, on_clean};
		}
		//对 同名函数 的简单包装
		[[nodiscard]] jobject NewGlobalRef(::jobject obj) const
		{
			//创建
			::jobject obj_2 = m_env->NewGlobalRef(obj);

			//简单检查
			assert_ok(obj_2);

			//准备函数：清理
			auto on_clean = [this, obj_2]()
			{
				//清理
				m_env->DeleteGlobalRef(obj_2);
			};

			//给出
			return {obj_2, on_clean};
		}
		//特化
		[[nodiscard]] jobject NewGlobalRef(const jobject & obj) const
		{
			//转发
			return NewGlobalRef(obj.get_native());
		}
		//特化
		[[nodiscard]] jclass NewGlobalRef(::jclass obj) const
		{
			//转发
			auto obj_2 = NewGlobalRef((::jobject)obj);

			//重新组装
			return {std::move((jclass &)obj_2)};
		}
		//特化
		[[nodiscard]] jclass NewGlobalRef(const jclass & obj) const
		{
			//转发
			return NewGlobalRef(obj.get_native());
		}
		//对 同名函数 的简单包装
		[[nodiscard]] jdoubleArray NewDoubleArray(jsize length) const
		{
			//创建
			::jdoubleArray obj_2 = m_env->NewDoubleArray(length);

			//简单检查
			assert_ok(obj_2);

			//准备函数：清理
			auto on_clean = [this, obj_2]()
			{
				//清理
				m_env->DeleteLocalRef(obj_2);
			};

			//给出
			return {obj_2, on_clean};
		}
		//对 同名函数 的简单包装
		[[nodiscard]] jclass FindClass(const std::u8string & name) const
		{
			//准备数据
			auto name_2 = (const char*)name.c_str();

			//获取类型
			::jclass class_1 = m_env->FindClass(name_2);

			//简单检查
			assert_ok(class_1);

			//准备函数：清理
			auto on_clean = [this, class_1]()
			{
				//清理
				m_env->DeleteLocalRef(class_1);
			};

			//给出
			return {class_1, on_clean};
		}
		//对 同名函数 的简单包装
		[[nodiscard]] jmethodID GetStaticMethodID(::jclass clazz, const std::u8string & name, const std::u8string & sig) const
		{
			//准备数据
			const auto name_2 = (const char*) name.c_str();
			const auto sig_2 = (const char*) sig.c_str();

			//获取ID
			::jmethodID method_id_1 = m_env->GetStaticMethodID(clazz, name_2, sig_2);

			//简单检查
			assert_ok(method_id_1);

			//给出
			return {method_id_1};
		}
		//特化
		[[nodiscard]] jmethodID GetStaticMethodID(const jclass & clazz, const std::u8string & name, const std::u8string & sig) const
		{
			//转发
			return GetStaticMethodID(clazz.get_native(), name, sig);
		}
		//对 同名函数 的简单包装
		template<typename ... Args>
		jobject CallStaticObjectMethod(::jclass clazz, ::jmethodID methodID, Args&&... args)
		{
			//转发
			::jobject return_value = m_env->CallStaticObjectMethod(
				clazz, methodID, std::forward<Args>(args)...);

			//准备函数：清理
			auto on_clean = [this, return_value]()
			{
				//清理
				m_env->DeleteLocalRef(return_value);
			};

			//给出
			return {return_value, on_clean};
		}
		//特化: 直接返回指定类型
		template<typename jobject_T, typename ... Args>
		jobject_T CallStaticObjectMethod(::jclass clazz, ::jmethodID methodID, Args&&... args)
		{
			//转发
			auto return_value = CallStaticObjectMethod(
				clazz, methodID, std::forward<Args>(args)...);

			//改变类型
			auto return_value_2 = std::move((jobject_T &)return_value);

			//给出
			return return_value_2;
		}
		//把 jstring 转为 std::u8string
		//note: 当 jstring 为 null 时，返回 "optional的空值"
		std::optional<std::u8string> get_text_from_jstring(const jstring & text)
		{
			//ai生成后修改

			//对象是 null，返回空值
			if (!text)
				return {};

			//获取文本数据
			const auto* text_2_data = m_env->GetStringChars(text.get_native(), nullptr);

			//简单检查
			utils_assert(text_2_data);

			//获取文本长度
			const auto text_2_length = m_env->GetStringLength(text.get_native());

			//保存为 std::u16string
			std::u16string text_3;
			text_3.assign((const char16_t *)text_2_data, text_2_length);

			//转为utf8
			const auto text_4 = utils::text::encoding::utf16_to_utf8(text_3);

			//释放资源
			m_env->ReleaseStringChars(text.get_native(), text_2_data);

			//给出
			return text_4;
		}

	private:
		//检查执行结果，如果失败，则清除异常后，报错
		void assert_ok(bool is_ok) const
		{
			//执行失败
			if (!is_ok)
			{
				//存在因此而新添加的java异常
				if (m_env->ExceptionCheck())
				{
					//清除异常
					m_env->ExceptionClear();
				}

				//报错
				utils_throw();
			}
		}
	};

	// 对 同名类 的简单包装
	class JavaVM
	{
	private:
		::JavaVM* const m_vm = nullptr;

	public:
		JavaVM(::JavaVM* vm)
			: m_vm{vm}
		{
			// 简单检查
			utils_assert(m_vm);
		}

	public:
		// 获得底层表示
		[[nodiscard]] ::JavaVM* get_native() const
		{
			return m_vm;
		}
		// 把当前线程，附加到JavaVM上
		[[nodiscard]] JNIEnv AttachCurrentThread() const
		{
			//准备数据
			::JNIEnv* env = nullptr;

			//执行操作
			utils_assert(m_vm->AttachCurrentThread(&env, nullptr) == JNI_OK);

			//准备函数：反向操作
			auto on_clean = [this]()
			{
				//执行操作
				utils_assert(m_vm->DetachCurrentThread() == JNI_OK);
			};

			//给出
			return {env, on_clean};
		}
	};
}