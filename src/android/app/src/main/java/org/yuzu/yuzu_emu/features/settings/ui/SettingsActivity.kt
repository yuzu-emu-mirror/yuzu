// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.features.settings.ui

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.Menu
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import android.view.ViewGroup.MarginLayoutParams
import androidx.core.view.updatePadding
import com.google.android.material.color.MaterialColors
import org.yuzu.yuzu_emu.NativeLibrary
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.databinding.ActivitySettingsBinding
import org.yuzu.yuzu_emu.features.settings.model.Settings
import org.yuzu.yuzu_emu.features.settings.model.SettingsViewModel
import org.yuzu.yuzu_emu.utils.*

class SettingsActivity : AppCompatActivity(), SettingsActivityView {
    private val presenter = SettingsActivityPresenter(this)

    private lateinit var binding: ActivitySettingsBinding

    private val settingsViewModel: SettingsViewModel by viewModels()

    override val settings: Settings get() = settingsViewModel.settings

    override fun onCreate(savedInstanceState: Bundle?) {
        ThemeHelper.setTheme(this)

        super.onCreate(savedInstanceState)

        binding = ActivitySettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        WindowCompat.setDecorFitsSystemWindows(window, false)

        val launcher = intent
        val gameID = launcher.getStringExtra(ARG_GAME_ID)
        val menuTag = launcher.getStringExtra(ARG_MENU_TAG)
        presenter.onCreate(savedInstanceState, menuTag!!, gameID!!)

        // Show "Back" button in the action bar for navigation
        setSupportActionBar(binding.toolbarSettings)
        supportActionBar!!.setDisplayHomeAsUpEnabled(true)

        if (InsetsHelper.getSystemGestureType(applicationContext) != InsetsHelper.GESTURE_NAVIGATION) {
            binding.navigationBarShade.setBackgroundColor(
                ThemeHelper.getColorWithOpacity(
                    MaterialColors.getColor(binding.navigationBarShade, R.attr.colorSurface),
                    ThemeHelper.SYSTEM_BAR_ALPHA
                )
            )
        }

        setInsets()
    }

    override fun onSupportNavigateUp(): Boolean {
        onBackPressed()
        return true
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        val inflater = menuInflater
        inflater.inflate(R.menu.menu_settings, menu)
        return true
    }

    override fun onSaveInstanceState(outState: Bundle) {
        // Critical: If super method is not called, rotations will be busted.
        super.onSaveInstanceState(outState)
        presenter.saveState(outState)
    }

    override fun onStart() {
        super.onStart()
        presenter.onStart()
    }

    /**
     * If this is called, the user has left the settings screen (potentially through the
     * home button) and will expect their changes to be persisted. So we kick off an
     * IntentService which will do so on a background thread.
     */
    override fun onStop() {
        super.onStop()
        presenter.onStop(isFinishing)

        // Update framebuffer layout when closing the settings
        NativeLibrary.notifyOrientationChange(
            EmulationMenuSettings.landscapeScreenLayout,
            windowManager.defaultDisplay.rotation
        )
    }

    override fun showSettingsFragment(menuTag: String, addToStack: Boolean, gameId: String) {
        if (!addToStack && settingsFragment != null) {
            return
        }

        val transaction = supportFragmentManager.beginTransaction()
        if (addToStack) {
            if (areSystemAnimationsEnabled()) {
                transaction.setCustomAnimations(
                    R.anim.anim_settings_fragment_in,
                    R.anim.anim_settings_fragment_out,
                    0,
                    R.anim.anim_pop_settings_fragment_out
                )
            }
            transaction.addToBackStack(null)
        }
        transaction.replace(
            R.id.frame_content,
            SettingsFragment.newInstance(menuTag, gameId),
            FRAGMENT_TAG
        )
        transaction.commit()
    }

    private fun areSystemAnimationsEnabled(): Boolean {
        val duration = android.provider.Settings.Global.getFloat(
            contentResolver,
            android.provider.Settings.Global.ANIMATOR_DURATION_SCALE, 1f
        )
        val transition = android.provider.Settings.Global.getFloat(
            contentResolver,
            android.provider.Settings.Global.TRANSITION_ANIMATION_SCALE, 1f
        )
        return duration != 0f && transition != 0f
    }

    override fun onSettingsFileLoaded() {
        val fragment: SettingsFragmentView? = settingsFragment
        fragment?.loadSettingsList()
    }

    override fun onSettingsFileNotFound() {
        val fragment: SettingsFragmentView? = settingsFragment
        fragment?.loadSettingsList()
    }

    override fun showToastMessage(message: String, is_long: Boolean) {
        Toast.makeText(
            this,
            message,
            if (is_long) Toast.LENGTH_LONG else Toast.LENGTH_SHORT
        ).show()
    }

    override fun onSettingChanged() {
        presenter.onSettingChanged()
    }

    private val settingsFragment: SettingsFragment?
        get() = supportFragmentManager.findFragmentByTag(FRAGMENT_TAG) as SettingsFragment?

    private fun setInsets() {
        ViewCompat.setOnApplyWindowInsetsListener(binding.frameContent) { view: View, windowInsets: WindowInsetsCompat ->
            val barInsets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
            val cutoutInsets = windowInsets.getInsets(WindowInsetsCompat.Type.displayCutout())
            view.updatePadding(
                left = barInsets.left + cutoutInsets.left,
                right = barInsets.right + cutoutInsets.right
            )

            val mlpAppBar = binding.appbarSettings.layoutParams as MarginLayoutParams
            mlpAppBar.leftMargin = barInsets.left + cutoutInsets.left
            mlpAppBar.rightMargin = barInsets.right + cutoutInsets.right
            binding.appbarSettings.layoutParams = mlpAppBar

            val mlpShade = binding.navigationBarShade.layoutParams as MarginLayoutParams
            mlpShade.height = barInsets.bottom
            binding.navigationBarShade.layoutParams = mlpShade

            windowInsets
        }
    }

    companion object {
        private const val ARG_MENU_TAG = "menu_tag"
        private const val ARG_GAME_ID = "game_id"
        private const val FRAGMENT_TAG = "settings"

        @JvmStatic
        fun launch(context: Context, menuTag: String?, gameId: String?) {
            val settings = Intent(context, SettingsActivity::class.java)
            settings.putExtra(ARG_MENU_TAG, menuTag)
            settings.putExtra(ARG_GAME_ID, gameId)
            context.startActivity(settings)
        }
    }
}
