// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.utils

import android.content.Context
import androidx.preference.PreferenceManager
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.features.settings.model.Settings

object MultiplayerHelper {
    fun initRoom(context: Context) {
        val preferences = PreferenceManager.getDefaultSharedPreferences(context)

        if(preferences.getBoolean(Settings.PREF_ROOM_CONNECT_ON_START, false)) {
            val addr = preferences.getString(Settings.PREF_ROOM_ADDRESS, "") ?: ""
            val port = preferences.getString(Settings.PREF_ROOM_PORT, "24872") ?: "24872"
            val nickname = preferences.getString(Settings.PREF_ROOM_NICKNAME, "") ?: ""
            val password = preferences.getString(Settings.PREF_ROOM_PASSWORD, "") ?: ""

            NativeLibrary.connectToRoom(nickname, addr, port.toIntOrNull() ?: 0, password)
        }
    }
}