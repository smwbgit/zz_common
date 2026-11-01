#pragma once

#include <memory>
#include <jni.h>
#include <utils/java/jni/JavaVM.hpp>

namespace utils::java::jni
{
	//负责把入口函数，传入的vm指针，保存起来
	class OnLoad
	{
	private:
		inline static std::unique_ptr<JavaVM> m_vm;

	public:
		inline static void set_vm(::JavaVM* vm)
		{
			//简单检查
			utils_assert(!m_vm);

			//保存
			m_vm = std::make_unique<JavaVM>(vm);
		}
		inline static JavaVM & get_vm()
		{
			//简单检查
			utils_assert(m_vm);

			//给出
			return *m_vm;
		}
	};
}

//入口函数
__attribute__((used))	//让编译器不要丢弃此函数
inline JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	//保存
	utils::java::jni::OnLoad::set_vm(vm);

	return JNI_VERSION_1_6;
}