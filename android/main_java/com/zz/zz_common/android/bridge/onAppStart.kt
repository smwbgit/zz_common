package com.zz.zz_common.android.bridge

import com.example.myapplication.MainActivity
import com.zz.zz_common.android.bridge.inner.WeakReferenceForMainActivity

// 初始化 桥
fun onAppStart(mainActivity: MainActivity)
{
    //准备数据
    val mainActivityRef = WeakReferenceForMainActivity()
    mainActivityRef.onAppStart(mainActivity)

    //初始化 每个桥
    com.zz.zz_common.android.bridge.Console.onAppStart(mainActivityRef)
    com.zz.zz_common.android.bridge.Location.onAppStart(mainActivityRef)
    com.zz.zz_common.android.bridge.Dirs.onAppStart(mainActivityRef)
}