package com.zz.zz_common.android.bridge

import androidx.annotation.Keep
import com.zz.zz_common.android.bridge.inner.WeakReferenceForMainActivity

//在界面上，添加一条消息
@Keep
object Console {
    private lateinit var mainActivityRef: WeakReferenceForMainActivity

    //初始化
    fun onAppStart(mainActivityRef: WeakReferenceForMainActivity)
    {
        //保存
        this.mainActivityRef = mainActivityRef
    }

    //添加一条消息
    @JvmStatic
    fun addMessage(message: String)
    {
        //获取对象
        val mainActivity = mainActivityRef.get()

        //添加消息
        mainActivity.addMessage(message)
    }
}