package org.yuzu.yuzu_emu.utils

import android.content.Context
import android.net.ConnectivityManager




object NetworkHelper {
    fun setRoutes(context: Context) {
        val connectivity =
            context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager

        val lp = connectivity.getLinkProperties(connectivity.activeNetwork)

        // lp.routes


    }
}