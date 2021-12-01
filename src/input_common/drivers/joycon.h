// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

#include <span>
#include <thread>

#include "input_common/helpers/joycon_protocol.h"
#include "input_common/input_engine.h"

namespace InputCommon {

class Joycons final : public InputCommon::InputEngine {
public:
    explicit Joycons(const std::string& input_engine_);

    ~Joycons();

    /// Used for automapping features
    std::vector<Common::ParamPackage> GetInputDevices() const override;
    ButtonMapping GetButtonMappingForDevice(const Common::ParamPackage& params) override;
    AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage& params) override;
    MotionMapping GetMotionMappingForDevice(const Common::ParamPackage& params) override;
    Common::Input::ButtonNames GetUIName(const Common::ParamPackage& params) const override;

    Common::Input::VibrationError SetRumble(
        const PadIdentifier& identifier, const Common::Input::VibrationStatus vibration) override;

private:
    enum class PadAxes {
        LeftStickX,
        LeftStickY,
        RightStickX,
        RightStickY,
        Undefined,
    };

    enum class PadMotion {
        LeftMotion,
        RightMotion,
        ProMotion,
        Undefined,
    };

    struct JoyconData {
        Joycon::JoyconHandle joycon_handle{nullptr, 0};

        // Harware config
        Joycon::GyroSensitivity gyro_sensitivity;
        Joycon::GyroPerformance gyro_performance;
        Joycon::AccelerometerSensitivity accelerometer_sensitivity;
        Joycon::AccelerometerPerformance accelerometer_performance;
        Joycon::ReportMode mode;
        bool imu_enabled;
        bool rumble_enabled;
        u8 leds;

        // Fixed value info
        f32 version;
        std::array<u8, 6> mac;
        Joycon::CalibrationData calibration;
        Joycon::ControllerType type;
        std::array<u8, 15> serial_number;
        Joycon::Color color;
        std::size_t port;

        // Polled data
        u32 raw_temperature;
        u8 raw_battery;
        Common::Input::VibrationStatus hd_rumble;
    };
    void InputThread(std::stop_token stop_token);
    void ScanThread(std::stop_token stop_token);

    /// Resets status of device connected to port
    void ResetDeviceType(std::size_t port);

    /// Captures JC Adapter endpoint address,
    void GetJCEndpoint();

    /// For shutting down, clear all data, join all threads, release usb
    void Reset();

    bool IsPayloadCorrect(int status, std::span<const u8> buffer);

    /// For use in initialization, querying devices to find the adapter
    void Setup();

    bool DeviceConnected(std::size_t port) const;

    void UpdateJoyconData(JoyconData& jc, std::span<const u8> buffer);
    void UpdateLeftPadInput(JoyconData& jc, std::span<const u8> buffer);
    void UpdateRightPadInput(JoyconData& jc, std::span<const u8> buffer);
    void UpdateProPadInput(JoyconData& jc, std::span<const u8> buffer);

    // Reads gyro and accelerometer values
    f32 GetAxisValue(u16 raw_value, Joycon::JoyStickAxisCalibration calibration) const;
    f32 GetAccelerometerValue(s16 raw_value, Joycon::ImuSensorCalibration cal,
                              Joycon::AccelerometerSensitivity sen) const;
    f32 GetGyroValue(s16 raw_value, Joycon::ImuSensorCalibration cal,
                     Joycon::GyroSensitivity sen) const;

    s16 GetRawIMUValues(size_t sensor, size_t axis, std::span<const u8> buffer) const;
    BasicMotion GetMotionInput(std::span<const u8> buffer, Joycon::ImuCalibration calibration,
                               Joycon::AccelerometerSensitivity accel_sensitivity,
                               Joycon::GyroSensitivity gyro_sensitivity) const;

    Joycon::JoyconHandle& GetHandle(PadIdentifier identifier);
    PadIdentifier GetIdentifier(std::size_t port) const;

    std::string JoyconName(std::size_t port) const;

    Common::Input::ButtonNames GetUIButtonName(const Common::ParamPackage& params) const;

    std::jthread input_thread;
    std::jthread scan_thread;
    bool input_thread_running{};
    bool scan_thread_running{};
    u8 input_error_counter{};

    std::array<JoyconData, 8> joycon_data{};
};

} // namespace InputCommon
