package com.zz.zz_common.android.bridge.inner

import com.example.myapplication.MainActivity
import com.zz.zz_common.android.utils_assert
import java.lang.ref.WeakReference

//对主模块的弱引用
class WeakReferenceForMainActivity {
    private lateinit var mainActivity: WeakReference<MainActivity>

    //初始化
    fun onAppStart(mainActivity: MainActivity)
    {
        //保存
        this.mainActivity = WeakReference(mainActivity)
    }

    //获取模块引用
    fun get(): MainActivity
    {
        //获取对象
        val mainActivity = this.mainActivity.get()

        //简单检查
        utils_assert(mainActivity != null && !mainActivity.isDestroyed)

        //给出
        return mainActivity!!
    }
}