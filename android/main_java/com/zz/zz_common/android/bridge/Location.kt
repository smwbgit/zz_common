package com.zz.zz_common.android.bridge

import android.content.Context.LOCATION_SERVICE
import androidx.annotation.Keep
import android.location.LocationManager
import android.os.SystemClock
import android.location.LocationListener
import android.os.Bundle
import android.os.Looper
import com.zz.zz_common.android.bridge.inner.WeakReferenceForMainActivity

@Keep
object Location {
    private lateinit var locationManager: LocationManager

    private lateinit var mainActivityRef: WeakReferenceForMainActivity

    //初始化
    fun onAppStart(mainActivityRef: WeakReferenceForMainActivity)
    {
        //保存
        this.mainActivityRef = mainActivityRef

        //获取对象
        val mainActivity = mainActivityRef.get()

        //获取位置管理器
        var appContext = mainActivity.applicationContext
        var locationManager = appContext.getSystemService(LOCATION_SERVICE)
                as LocationManager

        //保存
        this.locationManager = locationManager
    }

    //获取GPS位置
    @JvmStatic
    fun getLocation(out: DoubleArray): Int {
        val mgr = locationManager

        try {
            //获取GPS信息
            val location = mgr.getLastKnownLocation(LocationManager.GPS_PROVIDER)
            if (location == null)
                return -3

            //准备数据
            val maxAgeSeconds = 3
            val ageSec = (SystemClock.elapsedRealtimeNanos() - location.elapsedRealtimeNanos) / 1_000_000_000L

            //保存
            out[0] = location.latitude
            out[1] = location.longitude
            out[2] = location.altitude
            out[3] = ageSec.toDouble()
            out[4] = location.elapsedRealtimeNanos.toDouble() / 1_000_000_000L

            //准备数据
            val listener = object : LocationListener {
                override fun onLocationChanged(location: android.location.Location) {}
                override fun onProviderEnabled(provider: String) {}
                override fun onProviderDisabled(provider: String) {}
                override fun onStatusChanged(provider: String?, status: Int, extras: Bundle?) {}
            }

            //请求进行一次新的定位
            @Suppress("DEPRECATION")
            mgr.requestSingleUpdate(
                LocationManager.GPS_PROVIDER,
                listener,
                Looper.getMainLooper()
            )

            //完成
            return 0
        }
        catch(e: SecurityException) {
            return -4
        }
        catch(e: Exception) {
            return -5
        }
    }
}