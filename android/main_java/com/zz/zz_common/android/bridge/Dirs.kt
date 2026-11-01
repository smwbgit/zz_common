package com.zz.zz_common.android.bridge;

import androidx.annotation.Keep
import com.zz.zz_common.android.bridge.inner.WeakReferenceForMainActivity

//在界面上，添加一条消息
@Keep
object Dirs {

    private lateinit var mainActivityRef: WeakReferenceForMainActivity

    //初始化
    fun onAppStart(mainActivityRef: WeakReferenceForMainActivity)
    {
        //保存
        this.mainActivityRef = mainActivityRef
    }

    //获取，此app的，外部存储的，files目录的，完整路径
    //note：结果类似 “/storage/emulated/0/Android/data/com.example.myapplication/files”
    @JvmStatic
    fun getExternalFilesDir(): String?
    {
        //获取对象
        val mainActivity = mainActivityRef.get()

        //获取目录
        val dir = mainActivity.getExternalFilesDir(null)?.absolutePath

        //给出
        return dir
    }
}