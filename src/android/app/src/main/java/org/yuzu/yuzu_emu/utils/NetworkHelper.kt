// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.Context
import android.net.ConnectivityManager
import androidx.preference.PreferenceManager
import org.yuzu.yuzu_emu.features.settings.model.Settings

object NetworkHelper {
    /**
     * Gets available network interface info/route info - currently the active network info.
     * @return The route info separated by semicolons (interface, address, netmask, gateway), or null if no networks are available.
     */
    fun getRoute(context: Context): String? {
        val connectivity =
            context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager

        val preferences = PreferenceManager.getDefaultSharedPreferences(context)

        if(connectivity.isActiveNetworkMetered && preferences.getBoolean(Settings.PREF_FORCE_WIFI, false))
            return null

        val lp = connectivity.getLinkProperties(connectivity.activeNetwork) ?: return null

        val ifName = lp.interfaceName
        val addr = lp.linkAddresses[0]
        val cidr = addr.prefixLength

        val bits = 0xffffffff xor ((1 shl 32 - cidr)).toLong() - 1
        val mask = String.format(
            "%d.%d.%d.%d",
            bits and 0x0000000000ff000000L shr 24,
            bits and 0x000000000000ff0000L shr 16,
            bits and 0x00000000000000ff00L shr 8,
            bits and 0x0000000000000000ffL shr 0
        )

        val gw = lp.routes.last { it.isDefaultRoute }.gateway?.hostAddress

        return "$ifName;$addr;$mask;$gw"
    }
}