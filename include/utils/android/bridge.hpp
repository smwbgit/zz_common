#pragma once

#include <utils/java/jni/OnLoad.hpp>

// 对接器
namespace utils::android::bridge
{
	namespace inner
	{
		//java类中，一个静态函数
		class StaticFunction
		{
		private:
			std::optional<utils::java::jni::jclass> m_the_class;
			std::optional<utils::java::jni::jmethodID> m_the_method;

		public:
			//在app启动时，进行初始化
			void on_app_start(utils::java::jni::JNIEnv & env, const std::u8string & class_name, const std::u8string & method_name, const std::u8string & method_signature)
			{
				// 获取需要的信息，缓存起来

				// 获取信息
				auto theClass = env.FindClass(class_name);
				auto theMethod = env.GetStaticMethodID(theClass, method_name, method_signature);

				// 创建全局引用
				auto theClass_2 = env.NewGlobalRef(theClass);

				// 保存
				m_the_class = std::move(theClass_2);
				m_the_method = theMethod;
			}
			//在app退出时，进行清理
			void on_app_exit()
			{
				//释放引用
				m_the_class.reset();
				m_the_method.reset();
			}
			//进行调用
			template<typename ... Args>
			void call_void(utils::java::jni::JNIEnv & env, Args&&... args) const
			{
				//简单检查
				utils_assert(m_the_class.has_value());
				utils_assert(m_the_method.has_value());

				//进行调用
				env.get_native()->CallStaticVoidMethod(
					m_the_class->get_native(),
					m_the_method->get_native(),
					std::forward<Args>(args)...);
			}
			//进行调用
			template<typename ... Args>
			jint call_int(utils::java::jni::JNIEnv & env, Args&&... args) const
			{
				//简单检查
				utils_assert(m_the_class.has_value());
				utils_assert(m_the_method.has_value());

				//进行调用
				const auto return_value =
					env.get_native()->CallStaticIntMethod(
						m_the_class->get_native(),
						m_the_method->get_native(),
						std::forward<Args>(args)...);

				//给出
				return return_value;
			}
			//进行调用
			template<typename ... Args>
			utils::java::jni::jobject call_object(utils::java::jni::JNIEnv & env, Args&&... args) const
			{
				//简单检查
				utils_assert(m_the_class.has_value());
				utils_assert(m_the_method.has_value());

				//进行调用
				auto return_value =
					env.CallStaticObjectMethod<Args...>(
						m_the_class->get_native(),
						m_the_method->get_native(),
						std::forward<Args>(args)...);

				//给出
				return return_value;
			}
			//特化
			template<typename jobject_T, typename ... Args>
			jobject_T call_object(utils::java::jni::JNIEnv & env, Args&&... args) const
			{
				//简单检查
				utils_assert(m_the_class.has_value());
				utils_assert(m_the_method.has_value());

				//进行调用
				auto return_value =
					env.CallStaticObjectMethod<jobject_T, Args...>(
						m_the_class->get_native(),
						m_the_method->get_native(),
						std::forward<Args>(args)...);

				//给出
				return return_value;
			}
		};
	}

#pragma region 一个简易的控制台
	namespace inner::console
	{
		class Bridge
		{
		private:
			inner::StaticFunction m_add_message;

		public:
			//在app启动时，进行初始化
			void on_app_start(utils::java::jni::JNIEnv & env)
			{
				//转发
				m_add_message.on_app_start(
					env,
					u8"com/zz/zz_common/android/bridge/Console",
					u8"addMessage",
					u8"(Ljava/lang/String;)V");
			}
			//在app退出时，进行清理
			void on_app_exit()
			{
				//转发
				m_add_message.on_app_exit();
			}
			//添加一条消息
			void add_message(utils::java::jni::JNIEnv & env, const std::u8string & text)
			{
				//准备数据
				const auto text_2 = env.NewStringUTF(text);

				//进行调用
				m_add_message.call_void(env, text_2.get_native());
			}
		};
	}
	inline inner::console::Bridge console;
#pragma endregion

#pragma region GPS定位
	namespace inner::location
	{
		class Bridge
		{
		private:
			inner::StaticFunction m_get_location;

		public:
			// 在app启动时，进行初始化
			void on_app_start(utils::java::jni::JNIEnv & env)
			{
				//转发
				m_get_location.on_app_start(
					env,
					u8"com/zz/zz_common/android/bridge/Location",
					u8"getLocation",
					u8"([D)I");
			}
			// 在 app 退出时，进行清理
			void on_app_exit()
			{
				//转发
				m_get_location.on_app_exit();
			}
			// get_location 的查询结果
			struct s_gps_location
			{
				double latitude{};	//纬度
				double longitude{};	//经度
				double altitude{};	//高度
				double time_length_since_this_data{};	//此定位数据，到现在，已经经过的秒数
				double time_point_at_this_data{};		//此定位数据，的产生时间，（开机时间为起点，经过的秒数）
			};
			// 获得当前的GPS位置
			std::optional<s_gps_location> get_location(utils::java::jni::JNIEnv & env)
			{
				//创建 double[5] 数组作为输出参数
				auto out_array = env.NewDoubleArray(5);

				//进行调用
				const auto ret = m_get_location.call_int(env, out_array.get_native());

				//调用失败，返回空值
				if (ret)
					return {};

				//获取数组中的内容
				jdouble out[5]{};
				env.get_native()->GetDoubleArrayRegion(out_array.get_native(), 0, 5, out);

				//保存
				s_gps_location location{};
				location.latitude = out[0];
				location.longitude = out[1];
				location.altitude = out[2];
				location.time_length_since_this_data = out[3];
				location.time_point_at_this_data = out[4];

				//给出
				return location;
			}
		};
	}
	inline inner::location::Bridge location;
#pragma endregion

#pragma region 目录信息
	namespace inner::dirs
	{
		class Bridge
		{
		private:
			inner::StaticFunction m_get_external_files_dir;

		public:
			// 在app启动时，进行初始化
			void on_app_start(utils::java::jni::JNIEnv & env)
			{
				//转发
				m_get_external_files_dir.on_app_start(
					env,
					u8"com/zz/zz_common/android/bridge/Dirs",
					u8"getExternalFilesDir",
					u8"()Ljava/lang/String;");
			}
			// 在 app 退出时，进行清理
			void on_app_exit()
			{
				//转发
				m_get_external_files_dir.on_app_exit();
			}
			//获得，当前app，外部存储的，files文件夹的，绝对路径
			//return: 当调用失败时，返回空值
			std::optional<std::u8string> get_external_files_dir(utils::java::jni::JNIEnv & env)
			{
				//进行调用
				const auto dirname =
					m_get_external_files_dir.call_object<utils::java::jni::jstring>(env);

				//获取文本
				const auto dirname_2 = env.get_text_from_jstring(dirname);

				//给出
				return dirname_2;
			}
		};
	}
	inline inner::dirs::Bridge dirs;
#pragma endregion

	//在app启动时，进行初始化
	inline void on_app_start(utils::java::jni::JNIEnv & env)
	{
		utils::android::bridge::console.on_app_start(env);
		utils::android::bridge::location.on_app_start(env);
		utils::android::bridge::dirs.on_app_start(env);
	}
	//在app退出时，进行清理
	inline void on_app_exit()
	{
		utils::android::bridge::console.on_app_exit();
		utils::android::bridge::location.on_app_exit();
		utils::android::bridge::dirs.on_app_exit();
	}
}
