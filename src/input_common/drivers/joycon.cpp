// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include <fmt/format.h>

#include "common/param_package.h"
#include "common/settings.h"
#include "common/thread.h"
#include "input_common/drivers/joycon.h"

namespace InputCommon {

Joycons::Joycons(const std::string& input_engine_) : InputEngine(input_engine_) {
    if (Settings::values.enable_sdl_joycons) {
        // Avoid conflicting with SDL driver
        return;
    }
    LOG_INFO(Input, "Joycon driver Initialization started");
    const int init_res = SDL_hid_init();
    if (init_res == 0) {
        Setup();
    } else {
        LOG_ERROR(Input, "Hidapi could not be initialized. failed with error = {}", init_res);
    }
}

Joycons::~Joycons() {
    Reset();
}

void Joycons::UpdateJoyconData(JoyconData& jc, std::span<const u8> buffer) {
    switch (jc.type) {
    case Joycon::ControllerType::Left:
        UpdateLeftPadInput(jc, buffer);
        break;
    case Joycon::ControllerType::Right:
        UpdateRightPadInput(jc, buffer);
        break;
    case Joycon::ControllerType::Pro:
        UpdateProPadInput(jc, buffer);
        break;
    case Joycon::ControllerType::None:
        break;
    }
    jc.raw_battery = buffer[2] >> 4;
}

std::string Joycons::JoyconName(std::size_t port) const {
    switch (joycon_data[port].type) {
    case Joycon::ControllerType::Left:
        return "Left Joycon";
    case Joycon::ControllerType::Right:
        return "Right Joycon";
    case Joycon::ControllerType::Pro:
        return "Pro Controller";
    default:
        return "Unknow Joycon";
    }
}

void Joycons::ResetDeviceType(std::size_t port) {
    if (port > joycon_data.size()) {
        return;
    }
    joycon_data[port].type = Joycon::ControllerType::None;
}

void Joycons::InputThread(std::stop_token stop_token) {
    LOG_INFO(Input, "JC Adapter scan thread started");
    Common::SetCurrentThreadName("yuzu:input:JoyconInputThread");
    input_thread_running = true;
    std::vector<u8> buffer(Joycon::max_resp_size);

    while (!stop_token.stop_requested()) {
        for (std::size_t port = 0; port < joycon_data.size(); ++port) {
            if (joycon_data[port].type == Joycon::ControllerType::None) {
                continue;
            }
            const int status = SDL_hid_read_timeout(joycon_data[port].joycon_handle.handle,
                                                    buffer.data(), Joycon::max_resp_size, 15);

            if (IsPayloadCorrect(status, buffer)) {
                UpdateJoyconData(joycon_data[port], buffer);
            }
        }
        std::this_thread::yield();
    }
    input_thread_running = false;
}

bool Joycons::IsPayloadCorrect(int status, std::span<const u8> buffer) {
    if (status > 0) {
        if (buffer[0] == 0x00) {
            // Invalid response
            LOG_DEBUG(Input, "Error reading payload");
            if (input_error_counter++ > 20) {
                LOG_ERROR(Input, "Timeout, Is the joycon connected?");
                input_thread.request_stop();
            }
        }
        return false;
    }
    input_error_counter = 0;
    return true;
}

void Joycons::Setup() {
    // Initialize all controllers as unplugged
    for (std::size_t port = 0; port < joycon_data.size(); ++port) {
        PreSetController(GetIdentifier(port));
        joycon_data[port].type = Joycon::ControllerType::None;
        joycon_data[port].hd_rumble.high_amplitude = 0;
        joycon_data[port].hd_rumble.low_amplitude = 0;
        joycon_data[port].hd_rumble.high_frequency = 320.0f;
        joycon_data[port].hd_rumble.low_frequency = 160.0f;
    }
    if (!scan_thread_running) {
        scan_thread = std::jthread([this](std::stop_token stop_token) { ScanThread(stop_token); });
    }
}

void Joycons::ScanThread(std::stop_token stop_token) {
    Common::SetCurrentThreadName("yuzu:input:JoyconScanThread");
    scan_thread_running = true;
    std::size_t port = 0;
    // while (!stop_token.stop_requested()) {
    LOG_INFO(Input, "Scanning for devices");
    SDL_hid_device_info* devs = SDL_hid_enumerate(0x057e, 0x0);
    SDL_hid_device_info* cur_dev = devs;

    while (cur_dev) {
        LOG_WARNING(Input, "Device Found");
        LOG_WARNING(Input, "type : {:04X} {:04X}", cur_dev->vendor_id, cur_dev->product_id);
        bool skip_device = false;

        for (std::size_t i = 0; i < joycon_data.size(); ++i) {
            Joycon::ControllerType type{};
            const auto result = Joycon::GetDeviceType(cur_dev, type);

            if (result != Joycon::ErrorCode::Success || type == joycon_data[i].type) {
                LOG_WARNING(Input, "Device already exist, result = {}, type = {}", result, type);
                skip_device = true;
                break;
            }
        }
        if (!skip_device) {
            const auto result = Joycon::CheckDeviceAccess(joycon_data[port].joycon_handle, cur_dev);
            if (result == Joycon::ErrorCode::Success) {
                LOG_WARNING(Input, "Device verified and accessible");
                ++port;
            }
        }
        cur_dev = cur_dev->next;
    }
    GetJCEndpoint();

    //    std::this_thread::sleep_for(std::chrono::seconds(2));
    //}
    scan_thread_running = false;
}

void Joycons::Reset() {
    scan_thread = {};
    input_thread = {};

    for (std::size_t port = 0; port < joycon_data.size(); ++port) {
        joycon_data[port].type = Joycon::ControllerType::None;
        if (joycon_data[port].joycon_handle.handle) {
            joycon_data[port].joycon_handle.handle = nullptr;
            joycon_data[port].joycon_handle.packet_counter = 0;
        }
    }

    SDL_hid_exit();
}

void Joycons::GetJCEndpoint() {
    // init joycons
    for (std::size_t port = 0; port < joycon_data.size(); ++port) {
        auto& joycon = joycon_data[port];
        auto& handle = joycon.joycon_handle;
        joycon.port = port;
        if (!joycon.joycon_handle.handle) {
            continue;
        }
        LOG_INFO(Input, "Initializing device {}", port);

        Joycon::GetDeviceType(handle, joycon.type);
        // joycon_data[port].mac= Joycon::GetMac();
        // SetSerialNumber(joycon_data[port]);
        // SetVersionNumber(joycon_data[port]);
        // SetDeviceType(joycon_data[port]);
        Joycon::GetFactoryCalibrationData(handle, joycon.calibration, joycon.type);
        // GetColor(joycon_data[port]);

        joycon.rumble_enabled = true;
        joycon.imu_enabled = true;
        joycon.gyro_sensitivity = Joycon::GyroSensitivity::DPS2000;
        joycon.gyro_performance = Joycon::GyroPerformance::HZ833;
        joycon.accelerometer_sensitivity = Joycon::AccelerometerSensitivity::G8;
        joycon.accelerometer_performance = Joycon::AccelerometerPerformance::HZ100;

        Joycon::SetLedConfig(handle, static_cast<u8>(port + 1));
        Joycon::EnableRumble(handle, joycon.rumble_enabled);
        Joycon::EnableImu(handle, joycon.imu_enabled);
        Joycon::SetImuConfig(handle, joycon.gyro_sensitivity, joycon.gyro_performance,
                             joycon.accelerometer_sensitivity, joycon.accelerometer_performance);
        Joycon::SetReportMode(handle, Joycon::ReportMode::STANDARD_FULL_60HZ);
    }

    if (!input_thread_running) {
        input_thread =
            std::jthread([this](std::stop_token stop_token) { InputThread(stop_token); });
    }
}

bool Joycons::DeviceConnected(std::size_t port) const {
    if (port > joycon_data.size()) {
        return false;
    }
    return joycon_data[port].type != Joycon::ControllerType::None;
}

f32 Joycons::GetAxisValue(u16 raw_value, Joycon::JoyStickAxisCalibration calibration) const {
    const f32 value = static_cast<f32>(raw_value - calibration.center);
    if (value > 0.0f) {
        return value / calibration.max;
    }
    return value / calibration.min;
}

void Joycons::UpdateLeftPadInput(JoyconData& jc, std::span<const u8> buffer) {
    static constexpr std::array<Joycon::PadButton, 11> left_buttons{
        Joycon::PadButton::Down,    Joycon::PadButton::Up,     Joycon::PadButton::Right,
        Joycon::PadButton::Left,    Joycon::PadButton::LeftSL, Joycon::PadButton::LeftSR,
        Joycon::PadButton::L,       Joycon::PadButton::ZL,     Joycon::PadButton::Minus,
        Joycon::PadButton::Capture, Joycon::PadButton::StickL,
    };
    const auto identifier = GetIdentifier(jc.port);

    const u32 raw_button = static_cast<u32>(buffer[5] | ((buffer[4] & 0b00101001) << 16));
    for (std::size_t i = 0; i < left_buttons.size(); ++i) {
        const bool button_status = (raw_button & static_cast<u32>(left_buttons[i])) != 0;
        const int button = static_cast<int>(left_buttons[i]);
        SetButton(identifier, button, button_status);
    }

    const u16 raw_left_axis_x = static_cast<u16>(buffer[6] | ((buffer[7] & 0xf) << 8));
    const u16 raw_left_axis_y = static_cast<u16>((buffer[7] >> 4) | (buffer[8] << 4));
    const f32 left_axis_x = GetAxisValue(raw_left_axis_x, jc.calibration.left_stick.x);
    const f32 left_axis_y = GetAxisValue(raw_left_axis_y, jc.calibration.left_stick.y);

    SetAxis(identifier, static_cast<int>(PadAxes::LeftStickX), left_axis_x);
    SetAxis(identifier, static_cast<int>(PadAxes::LeftStickY), left_axis_y);

    if (jc.imu_enabled) {
        auto left_motion = GetMotionInput(buffer, jc.calibration.imu, jc.accelerometer_sensitivity,
                                          jc.gyro_sensitivity);
        // Rotate motion axis to the correct direction
        left_motion.accel_y = -left_motion.accel_y;
        left_motion.accel_z = -left_motion.accel_z;
        left_motion.gyro_x = -left_motion.gyro_x;
        SetMotion(identifier, static_cast<int>(PadMotion::LeftMotion), left_motion);
    }
}

void Joycons::UpdateRightPadInput(JoyconData& jc, std::span<const u8> buffer) {
    static constexpr std::array<Joycon::PadButton, 11> right_buttons{
        Joycon::PadButton::Y,    Joycon::PadButton::X,       Joycon::PadButton::B,
        Joycon::PadButton::A,    Joycon::PadButton::RightSL, Joycon::PadButton::RightSR,
        Joycon::PadButton::R,    Joycon::PadButton::ZR,      Joycon::PadButton::Plus,
        Joycon::PadButton::Home, Joycon::PadButton::StickR,
    };
    const auto identifier = GetIdentifier(jc.port);

    const u32 raw_button = static_cast<u32>((buffer[3] << 8) | (buffer[4] << 16));
    for (std::size_t i = 0; i < right_buttons.size(); ++i) {
        const bool button_status = (raw_button & static_cast<u32>(right_buttons[i])) != 0;
        const int button = static_cast<int>(right_buttons[i]);
        SetButton(identifier, button, button_status);
    }

    const u16 raw_right_axis_x = static_cast<u16>(buffer[9] | ((buffer[10] & 0xf) << 8));
    const u16 raw_right_axis_y = static_cast<u16>((buffer[10] >> 4) | (buffer[11] << 4));
    const f32 right_axis_x = GetAxisValue(raw_right_axis_x, jc.calibration.right_stick.x);
    const f32 right_axis_y = GetAxisValue(raw_right_axis_y, jc.calibration.right_stick.y);
    SetAxis(identifier, static_cast<int>(PadAxes::RightStickX), right_axis_x);
    SetAxis(identifier, static_cast<int>(PadAxes::RightStickY), right_axis_y);

    if (jc.imu_enabled) {
        auto right_motion = GetMotionInput(buffer, jc.calibration.imu, jc.accelerometer_sensitivity,
                                           jc.gyro_sensitivity);
        // Rotate motion axis to the correct direction
        right_motion.accel_x = -right_motion.accel_x;
        right_motion.accel_y = -right_motion.accel_y;
        right_motion.gyro_z = -right_motion.gyro_z;
        SetMotion(identifier, static_cast<int>(PadMotion::RightMotion), right_motion);
    }
}

void Joycons::UpdateProPadInput(JoyconData& jc, std::span<const u8> buffer) {
    static constexpr std::array<Joycon::PadButton, 18> pro_buttons{
        Joycon::PadButton::Down,  Joycon::PadButton::Up,      Joycon::PadButton::Right,
        Joycon::PadButton::Left,  Joycon::PadButton::L,       Joycon::PadButton::ZL,
        Joycon::PadButton::Minus, Joycon::PadButton::Capture, Joycon::PadButton::Y,
        Joycon::PadButton::X,     Joycon::PadButton::B,       Joycon::PadButton::A,
        Joycon::PadButton::R,     Joycon::PadButton::ZR,      Joycon::PadButton::Plus,
        Joycon::PadButton::Home,  Joycon::PadButton::StickL,  Joycon::PadButton::StickR,
    };
    const auto identifier = GetIdentifier(jc.port);

    const u32 raw_button = static_cast<u32>(buffer[5] | (buffer[3] << 8) | (buffer[4] << 16));
    for (std::size_t i = 0; i < pro_buttons.size(); ++i) {
        const bool button_status = (raw_button & static_cast<u32>(pro_buttons[i])) != 0;
        const int button = static_cast<int>(pro_buttons[i]);
        SetButton(identifier, button, button_status);
    }

    const u16 raw_left_axis_x = static_cast<u16>(buffer[6] | ((buffer[7] & 0xf) << 8));
    const u16 raw_left_axis_y = static_cast<u16>((buffer[7] >> 4) | (buffer[8] << 4));
    const u16 raw_right_axis_x = static_cast<u16>(buffer[9] | ((buffer[10] & 0xf) << 8));
    const u16 raw_right_axis_y = static_cast<u16>((buffer[10] >> 4) | (buffer[11] << 4));
    const f32 left_axis_x = GetAxisValue(raw_left_axis_x, jc.calibration.left_stick.x);
    const f32 left_axis_y = GetAxisValue(raw_left_axis_y, jc.calibration.left_stick.y);
    const f32 right_axis_x = GetAxisValue(raw_right_axis_x, jc.calibration.right_stick.x);
    const f32 right_axis_y = GetAxisValue(raw_right_axis_y, jc.calibration.right_stick.y);
    SetAxis(identifier, static_cast<int>(PadAxes::LeftStickX), left_axis_x);
    SetAxis(identifier, static_cast<int>(PadAxes::LeftStickY), left_axis_y);
    SetAxis(identifier, static_cast<int>(PadAxes::RightStickX), right_axis_x);
    SetAxis(identifier, static_cast<int>(PadAxes::RightStickY), right_axis_y);

    if (jc.imu_enabled) {
        auto pro_motion = GetMotionInput(buffer, jc.calibration.imu, jc.accelerometer_sensitivity,
                                         jc.gyro_sensitivity);
        pro_motion.accel_y = -pro_motion.accel_y;
        pro_motion.accel_z = -pro_motion.accel_z;
        SetMotion(identifier, static_cast<int>(PadMotion::ProMotion), pro_motion);
    }
}

f32 Joycons::GetAccelerometerValue(s16 raw, Joycon::ImuSensorCalibration cal,
                                   Joycon::AccelerometerSensitivity sen) const {
    const f32 value = raw * (1.0f / (cal.scale - cal.offset)) * 4;
    switch (sen) {
    case Joycon::AccelerometerSensitivity::G2:
        return value / 4.0f;
    case Joycon::AccelerometerSensitivity::G4:
        return value / 2.0f;
    case Joycon::AccelerometerSensitivity::G8:
        return value;
    case Joycon::AccelerometerSensitivity::G16:
        return value * 2.0f;
    }
    return value;
}

f32 Joycons::GetGyroValue(s16 raw, Joycon::ImuSensorCalibration cal,
                          Joycon::GyroSensitivity sen) const {
    const f32 value = (raw - cal.offset) * (936.0f / (cal.scale - cal.offset)) / 360.0f;
    switch (sen) {
    case Joycon::GyroSensitivity::DPS250:
        return value / 8.0f;
    case Joycon::GyroSensitivity::DPS500:
        return value / 4.0f;
    case Joycon::GyroSensitivity::DPS1000:
        return value / 2.0f;
    case Joycon::GyroSensitivity::DPS2000:
        return value;
    }
    return value;
}

s16 Joycons::GetRawIMUValues(std::size_t sensor, size_t axis, std::span<const u8> buffer) const {
    const size_t offset = (sensor * 6) + (axis * 2);
    return static_cast<s16>(buffer[13 + offset] | (buffer[14 + offset] << 8));
}

BasicMotion Joycons::GetMotionInput(std::span<const u8> buffer, Joycon::ImuCalibration calibration,
                                    Joycon::AccelerometerSensitivity accel_sensitivity,
                                    Joycon::GyroSensitivity gyro_sensitivity) const {
    std::array<BasicMotion, 3> motion_data{};
    for (std::size_t sample = 0; sample < motion_data.size(); ++sample) {
        auto& motion = motion_data[sample];
        const auto& accel_cal = calibration.accelerometer;
        const auto& gyro_cal = calibration.gyro;
        const std::size_t sample_index = sample * 2;
        const s16 raw_accel_x = GetRawIMUValues(sample_index, 1, buffer);
        const s16 raw_accel_y = GetRawIMUValues(sample_index, 0, buffer);
        const s16 raw_accel_z = GetRawIMUValues(sample_index, 2, buffer);
        const s16 raw_gyro_x = GetRawIMUValues(sample_index + 1, 1, buffer);
        const s16 raw_gyro_y = GetRawIMUValues(sample_index + 1, 0, buffer);
        const s16 raw_gyro_z = GetRawIMUValues(sample_index + 1, 2, buffer);

        motion.delta_timestamp = 15000;
        motion.accel_x = GetAccelerometerValue(raw_accel_x, accel_cal[1], accel_sensitivity);
        motion.accel_y = GetAccelerometerValue(raw_accel_y, accel_cal[0], accel_sensitivity);
        motion.accel_z = GetAccelerometerValue(raw_accel_z, accel_cal[2], accel_sensitivity);
        motion.gyro_x = GetGyroValue(raw_gyro_x, gyro_cal[1], gyro_sensitivity);
        motion.gyro_y = GetGyroValue(raw_gyro_y, gyro_cal[0], gyro_sensitivity);
        motion.gyro_z = GetGyroValue(raw_gyro_z, gyro_cal[2], gyro_sensitivity);
    }

    // TODO(German77): Return all three samples data
    return motion_data[0];
}

Common::Input::VibrationError Joycons::SetRumble(const PadIdentifier& identifier,
                                                 const Common::Input::VibrationStatus& vibration) {
    const Joycon::VibrationValue native_vibration{
        .low_amplitude = vibration.low_amplitude,
        .low_frequency = vibration.low_frequency,
        .high_amplitude = vibration.high_amplitude,
        .high_frequency = vibration.high_amplitude,
    };
    Joycon::SetVibration(GetHandle(identifier), native_vibration);
    return Common::Input::VibrationError::None;
}

Joycon::JoyconHandle& Joycons::GetHandle(PadIdentifier identifier) {
    return joycon_data[identifier.port].joycon_handle;
}

PadIdentifier Joycons::GetIdentifier(std::size_t port) const {
    return {
        .guid = Common::UUID{Common::InvalidUUID},
        .port = port,
        .pad = 0,
    };
}

std::vector<Common::ParamPackage> Joycons::GetInputDevices() const {
    std::vector<Common::ParamPackage> devices;
    for (const auto& controller : joycon_data) {
        if (DeviceConnected(controller.port)) {
            std::string name = fmt::format("{} {}", JoyconName(controller.port), controller.port);
            devices.emplace_back(Common::ParamPackage{
                {"engine", GetEngineName()},
                {"display", std::move(name)},
                {"port", std::to_string(controller.port)},
            });
        }
    }
    return devices;
}

ButtonMapping Joycons::GetButtonMappingForDevice(const Common::ParamPackage& params) {
    static constexpr std::array<std::pair<Settings::NativeButton::Values, Joycon::PadButton>, 20>
        switch_to_joycon_button = {
            std::pair{Settings::NativeButton::A, Joycon::PadButton::A},
            {Settings::NativeButton::B, Joycon::PadButton::B},
            {Settings::NativeButton::X, Joycon::PadButton::X},
            {Settings::NativeButton::Y, Joycon::PadButton::Y},
            {Settings::NativeButton::DLeft, Joycon::PadButton::Left},
            {Settings::NativeButton::DUp, Joycon::PadButton::Up},
            {Settings::NativeButton::DRight, Joycon::PadButton::Right},
            {Settings::NativeButton::DDown, Joycon::PadButton::Down},
            {Settings::NativeButton::SL, Joycon::PadButton::LeftSL},
            {Settings::NativeButton::SR, Joycon::PadButton::LeftSR},
            {Settings::NativeButton::L, Joycon::PadButton::L},
            {Settings::NativeButton::R, Joycon::PadButton::R},
            {Settings::NativeButton::ZL, Joycon::PadButton::ZL},
            {Settings::NativeButton::ZR, Joycon::PadButton::ZR},
            {Settings::NativeButton::Plus, Joycon::PadButton::Plus},
            {Settings::NativeButton::Minus, Joycon::PadButton::Minus},
            {Settings::NativeButton::Home, Joycon::PadButton::Home},
            {Settings::NativeButton::Screenshot, Joycon::PadButton::Capture},
            {Settings::NativeButton::LStick, Joycon::PadButton::StickL},
            {Settings::NativeButton::RStick, Joycon::PadButton::StickR},
        };

    if (!params.Has("port")) {
        return {};
    }

    ButtonMapping mapping{};
    for (const auto& [switch_button, joycon_button] : switch_to_joycon_button) {
        Common::ParamPackage button_params{};
        button_params.Set("engine", GetEngineName());
        button_params.Set("port", params.Get("port", 0));
        button_params.Set("button", static_cast<int>(joycon_button));
        mapping.insert_or_assign(switch_button, std::move(button_params));
    }

    return mapping;
}

AnalogMapping Joycons::GetAnalogMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("port")) {
        return {};
    }

    AnalogMapping mapping = {};
    Common::ParamPackage left_analog_params;
    left_analog_params.Set("engine", GetEngineName());
    left_analog_params.Set("port", params.Get("port", 0));
    left_analog_params.Set("axis_x", static_cast<int>(PadAxes::LeftStickX));
    left_analog_params.Set("axis_y", static_cast<int>(PadAxes::LeftStickY));
    mapping.insert_or_assign(Settings::NativeAnalog::LStick, std::move(left_analog_params));
    Common::ParamPackage right_analog_params;
    right_analog_params.Set("engine", GetEngineName());
    right_analog_params.Set("port", params.Get("port", 0));
    right_analog_params.Set("axis_x", static_cast<int>(PadAxes::RightStickX));
    right_analog_params.Set("axis_y", static_cast<int>(PadAxes::RightStickY));
    mapping.insert_or_assign(Settings::NativeAnalog::RStick, std::move(right_analog_params));
    return mapping;
}

MotionMapping Joycons::GetMotionMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("port")) {
        return {};
    }

    MotionMapping mapping = {};
    Common::ParamPackage left_motion_params;
    left_motion_params.Set("engine", GetEngineName());
    left_motion_params.Set("port", params.Get("port", 0));
    left_motion_params.Set("motion", 0);
    mapping.insert_or_assign(Settings::NativeMotion::MotionLeft, std::move(left_motion_params));
    Common::ParamPackage right_Motion_params;
    right_Motion_params.Set("engine", GetEngineName());
    right_Motion_params.Set("port", params.Get("port", 0));
    right_Motion_params.Set("motion", 1);
    mapping.insert_or_assign(Settings::NativeMotion::MotionRight, std::move(right_Motion_params));
    return mapping;
}

Common::Input::ButtonNames Joycons::GetUIButtonName(const Common::ParamPackage& params) const {
    const auto button = static_cast<Joycon::PadButton>(params.Get("button", 0));
    switch (button) {
    case Joycon::PadButton::Left:
        return Common::Input::ButtonNames::ButtonLeft;
    case Joycon::PadButton::Right:
        return Common::Input::ButtonNames::ButtonRight;
    case Joycon::PadButton::Down:
        return Common::Input::ButtonNames::ButtonDown;
    case Joycon::PadButton::Up:
        return Common::Input::ButtonNames::ButtonUp;
    case Joycon::PadButton::LeftSL:
    case Joycon::PadButton::RightSL:
        return Common::Input::ButtonNames::TriggerSL;
    case Joycon::PadButton::LeftSR:
    case Joycon::PadButton::RightSR:
        return Common::Input::ButtonNames::TriggerSR;
    case Joycon::PadButton::L:
        return Common::Input::ButtonNames::TriggerL;
    case Joycon::PadButton::R:
        return Common::Input::ButtonNames::TriggerR;
    case Joycon::PadButton::ZL:
        return Common::Input::ButtonNames::TriggerZL;
    case Joycon::PadButton::ZR:
        return Common::Input::ButtonNames::TriggerZR;
    case Joycon::PadButton::A:
        return Common::Input::ButtonNames::ButtonA;
    case Joycon::PadButton::B:
        return Common::Input::ButtonNames::ButtonB;
    case Joycon::PadButton::X:
        return Common::Input::ButtonNames::ButtonX;
    case Joycon::PadButton::Y:
        return Common::Input::ButtonNames::ButtonY;
    case Joycon::PadButton::Plus:
        return Common::Input::ButtonNames::ButtonPlus;
    case Joycon::PadButton::Minus:
        return Common::Input::ButtonNames::ButtonMinus;
    case Joycon::PadButton::Home:
        return Common::Input::ButtonNames::ButtonHome;
    case Joycon::PadButton::Capture:
        return Common::Input::ButtonNames::ButtonCapture;
    case Joycon::PadButton::StickL:
        return Common::Input::ButtonNames::ButtonStickL;
    case Joycon::PadButton::StickR:
        return Common::Input::ButtonNames::ButtonStickR;
    default:
        return Common::Input::ButtonNames::Undefined;
    }
}

Common::Input::ButtonNames Joycons::GetUIName(const Common::ParamPackage& params) const {
    if (params.Has("button")) {
        return GetUIButtonName(params);
    }
    if (params.Has("axis")) {
        return Common::Input::ButtonNames::Value;
    }
    if (params.Has("motion")) {
        return Common::Input::ButtonNames::Engine;
    }

    return Common::Input::ButtonNames::Invalid;
}

} // namespace InputCommon
