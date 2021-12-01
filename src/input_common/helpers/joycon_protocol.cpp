// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include <math.h>

#include "common/logging/log.h"
#include "input_common/helpers/joycon_protocol.h"

namespace InputCommon::Joycon {

u8 GetCounter(JoyconHandle& joycon_handle) {
    joycon_handle.packet_counter = (joycon_handle.packet_counter + 1) & 0x0F;
    return joycon_handle.packet_counter;
}

bool CheckDeviceAccess(JoyconHandle& joycon_handle, SDL_hid_device_info* device_info) {
    if (GetDeviceType(device_info) == ControllerType::None) {
        return false;
    }

    joycon_handle.handle =
        SDL_hid_open(device_info->vendor_id, device_info->product_id, device_info->serial_number);
    if (!joycon_handle.handle) {
        LOG_ERROR(Input, "Yuzu can't gain access to this device: ID {:04X}:{:04X}.",
                  device_info->vendor_id, device_info->product_id);
        return false;
    }
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return true;
}

f32 EncodeRumbleAmplification(f32 amplification) {
    if (amplification < 0.01182) {
        return roundf(powf(amplification, 1.7f) * 7561);
    } else if (amplification < 0.11249) {
        return roundf((logf(amplification) * 11.556f) + 55.3f);
    } else if (amplification < 0.22498) {
        return roundf((log2f(amplification) * 32) + 131);
    }
    return roundf((log2f(amplification) * 64) + 200);
}

void SetVibration(JoyconHandle& joycon_handle, VibrationValue vibration) {
    std::vector<u8> buffer(Joycon::max_resp_size);

    buffer[0] = static_cast<u8>(Joycon::Output::RUMBLE_ONLY);
    buffer[1] = GetCounter(joycon_handle);

    if (vibration.high_amplitude == 0.0f && vibration.low_amplitude == 0.0f) {
        for (std::size_t i = 0; i < Joycon::default_buffer.size(); ++i) {
            buffer[2 + i] = Joycon::default_buffer[i];
        }
        SendData(joycon_handle, buffer, max_resp_size);
        return;
    }

    const u16 encoded_hf =
        static_cast<u16>(roundf(128 * log2f(vibration.high_frequency * 0.1f)) - 0x180);
    const u8 encoded_lf =
        static_cast<u8>(roundf(32 * log2f(vibration.low_frequency * 0.1f)) - 0x40);
    const u8 encoded_hamp = static_cast<u8>(EncodeRumbleAmplification(vibration.high_amplitude));
    const u8 encoded_lamp = static_cast<u8>(EncodeRumbleAmplification(vibration.low_amplitude));

    for (u8 i = 0; i < 2; ++i) {
        const u8 amplitude = i == 0 ? encoded_lamp : encoded_hamp;
        const u8 offset = i * 4;
        u32 encoded_amp = amplitude >> 1;
        const u32 parity = (encoded_amp % 2) * 0x80;
        encoded_amp >>= 1;
        encoded_amp += 0x40;

        buffer[2 + offset] = static_cast<u8>(encoded_hf & 0xff);
        buffer[3 + offset] = static_cast<u8>((encoded_hf >> 8) & 0xff);
        buffer[4 + offset] = static_cast<u8>(encoded_lf & 0xff);

        buffer[3 + offset] |= static_cast<u8>(amplitude);
        buffer[4 + offset] |= static_cast<u8>(parity);
        buffer[5 + offset] = static_cast<u8>(encoded_amp);
    }

    SendData(joycon_handle, buffer, max_resp_size);
}

void SetImuConfig(JoyconHandle& joycon_handle, GyroSensitivity gsen, GyroPerformance gfrec,
                  AccelerometerSensitivity asen, AccelerometerPerformance afrec) {
    const std::vector<u8> buffer{static_cast<u8>(gsen), static_cast<u8>(asen),
                                 static_cast<u8>(gfrec), static_cast<u8>(afrec)};
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    SendSubCommand(joycon_handle, SubCommand::SET_IMU_SENSITIVITY, buffer, 4);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
}

void SetLedConfig(JoyconHandle& joycon_handle, u8 leds) {
    const std::vector<u8> buffer{leds};
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    SendSubCommand(joycon_handle, SubCommand::SET_PLAYER_LIGHTS, buffer, 1);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
}

void SetReportMode(JoyconHandle& joycon_handle, ReportMode report_mode) {
    const std::vector<u8> buffer{static_cast<u8>(report_mode)};
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    SendSubCommand(joycon_handle, SubCommand::SET_REPORT_MODE, buffer, 1);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
}

Color GetColor(JoyconHandle& joycon_handle) {
    Color joycon_color;
    std::vector<u8> buffer;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    buffer = ReadSPI(joycon_handle, Joycon::CalAddr::COLOR_DATA, 12);
    joycon_color.body = static_cast<u32>((buffer[2] << 16) | (buffer[1] << 8) | buffer[0]);
    joycon_color.buttons = static_cast<u32>((buffer[5] << 16) | (buffer[4] << 8) | buffer[3]);
    joycon_color.left_grip = static_cast<u32>((buffer[8] << 16) | (buffer[7] << 8) | buffer[6]);
    joycon_color.right_grip = static_cast<u32>((buffer[11] << 16) | (buffer[10] << 8) | buffer[9]);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return joycon_color;
}

std::array<u8, 6> GetMacAddress(JoyconHandle& joycon_handle) {
    std::array<u8, 6> mac_address{};
    wchar_t buffer[255];
    const auto result =
        SDL_hid_get_serial_number_string(joycon_handle.handle, buffer, std::size(buffer));
    if (result == -1) {
        LOG_ERROR(Input, "Unable to get mac address");
        return mac_address;
    }
    for (std::size_t i = 0; i < mac_address.size(); ++i) {
        wchar_t value[3] = {buffer[i * 2], buffer[(i * 2) + 1]};
        mac_address[i] = static_cast<u8>(std::stoi(value, 0, 16));
    }
    return mac_address;
}

std::array<u8, 15> GetSerialNumber(JoyconHandle& joycon_handle) {
    std::array<u8, 15> serial_number;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const std::vector<u8> buffer = ReadSPI(joycon_handle, Joycon::CalAddr::SERIAL_NUMBER, 16);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    for (std::size_t i = 0; i < serial_number.size(); ++i) {
        serial_number[i] = buffer[i + 1];
    }
    return serial_number;
}

ControllerType GetDeviceType(SDL_hid_device_info* device_info) {
    if (device_info->vendor_id != 0x057e) {
        return ControllerType::None;
    }
    switch (device_info->product_id) {
    case 0x2006:
        return ControllerType::Left;
    case 0x2007:
        return ControllerType::Right;
    case 0x2009:
        return ControllerType::Pro;
    default:
        return ControllerType::None;
    }
}

ControllerType GetDeviceType(JoyconHandle& joycon_handle) {
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const std::vector<u8> buffer = ReadSPI(joycon_handle, Joycon::CalAddr::DEVICE_TYPE, 1);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return static_cast<Joycon::ControllerType>(buffer[0]);
}

u16 GetVersionNumber([[maybe_unused]] JoyconHandle& joycon_handle) {
    // Not implemented
    return 0;
}

JoyStickCalibration GetLeftJoyStickCalibration(JoyconHandle& joycon_handle,
                                               bool is_factory_calibration) {
    JoyStickCalibration joystick{};
    std::vector<u8> buffer;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    if (is_factory_calibration) {
        buffer = ReadSPI(joycon_handle, CalAddr::FACT_LEFT_DATA, 9);
    } else {
        buffer = ReadSPI(joycon_handle, CalAddr::USER_LEFT_DATA, 9);
    }
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);

    joystick.x.max = static_cast<u16>(((buffer[1] & 0x0F) << 8) | buffer[0]);
    joystick.y.max = static_cast<u16>((buffer[2] << 4) | (buffer[1] >> 4));
    joystick.x.center = static_cast<u16>(((buffer[4] & 0x0F) << 8) | buffer[3]);
    joystick.y.center = static_cast<u16>((buffer[5] << 4) | (buffer[4] >> 4));
    joystick.x.min = static_cast<u16>(((buffer[7] & 0x0F) << 8) | buffer[6]);
    joystick.y.min = static_cast<u16>((buffer[8] << 4) | (buffer[7] >> 4));

    // Nintendo fix for drifting stick
    // buffer = ReadSPI(0x60, 0x86 , 16);
    // joystick.deadzone = (u16)((buffer[4] << 8) & 0xF00 | buffer[3]);
    return joystick;
}

JoyStickCalibration GetRightJoyStickCalibration(JoyconHandle& joycon_handle,
                                                bool is_factory_calibration) {
    JoyStickCalibration joystick{};
    std::vector<u8> buffer;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    if (is_factory_calibration) {
        buffer = ReadSPI(joycon_handle, CalAddr::FACT_RIGHT_DATA, 9);
    } else {
        buffer = ReadSPI(joycon_handle, CalAddr::USER_RIGHT_DATA, 9);
    }
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);

    joystick.x.center = static_cast<u16>(((buffer[1] & 0x0F) << 8) | buffer[0]);
    joystick.y.center = static_cast<u16>((buffer[2] << 4) | (buffer[1] >> 4));
    joystick.x.min = static_cast<u16>(((buffer[4] & 0x0F) << 8) | buffer[3]);
    joystick.y.min = static_cast<u16>((buffer[5] << 4) | (buffer[4] >> 4));
    joystick.x.max = static_cast<u16>(((buffer[7] & 0x0F) << 8) | buffer[6]);
    joystick.y.max = static_cast<u16>((buffer[8] << 4) | (buffer[7] >> 4));

    // Nintendo fix for drifting stick
    // buffer = ReadSPI(0x60, 0x98 , 16);
    // joystick.deadzone = (u16)((buffer[4] << 8) & 0xF00 | buffer[3]);
    return joystick;
}

ImuCalibration GetImuCalibration(JoyconHandle& joycon_handle, bool is_factory_calibration) {
    ImuCalibration imu{};
    std::vector<u8> buffer;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    if (is_factory_calibration) {
        buffer = ReadSPI(joycon_handle, CalAddr::FACT_IMU_DATA, 24);
    } else {
        buffer = ReadSPI(joycon_handle, CalAddr::USER_IMU_DATA, 24);
    }
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    for (std::size_t i = 0; i < imu.accelerometer.size(); ++i) {
        const std::size_t index = i * 2;
        imu.accelerometer[i].offset = static_cast<s16>(buffer[index] | (buffer[index + 1] << 8));
        imu.accelerometer[i].scale = static_cast<s16>(buffer[index + 6] | (buffer[index + 7] << 8));
    }
    for (std::size_t i = 0; i < imu.gyro.size(); ++i) {
        const std::size_t index = i * 2;
        imu.gyro[i].offset = static_cast<s16>(buffer[index + 12] | (buffer[index + 13] << 8));
        imu.gyro[i].scale = static_cast<s16>(buffer[index + 18] | (buffer[index + 19] << 8));
    }
    return imu;
}

CalibrationData GetUserCalibrationData(JoyconHandle& joycon_handle,
                                       ControllerType controller_type) {
    CalibrationData calibration{};
    switch (controller_type) {
    case ControllerType::Left:
        calibration.left_stick = GetLeftJoyStickCalibration(joycon_handle, false);
        break;
    case ControllerType::Right:
        calibration.right_stick = GetRightJoyStickCalibration(joycon_handle, false);
        break;
    case Joycon::ControllerType::Pro:
        calibration.left_stick = GetLeftJoyStickCalibration(joycon_handle, false);
        calibration.right_stick = GetRightJoyStickCalibration(joycon_handle, false);
        break;
    default:
        break;
    }

    calibration.imu = GetImuCalibration(joycon_handle, false);
    return calibration;
}

CalibrationData GetFactoryCalibrationData(JoyconHandle& joycon_handle,
                                          ControllerType controller_type) {
    CalibrationData calibration{};
    switch (controller_type) {
    case ControllerType::Left:
        calibration.left_stick = GetLeftJoyStickCalibration(joycon_handle, true);
        break;
    case ControllerType::Right:
        calibration.right_stick = GetRightJoyStickCalibration(joycon_handle, true);
        break;
    case Joycon::ControllerType::Pro:
        calibration.left_stick = GetLeftJoyStickCalibration(joycon_handle, true);
        calibration.right_stick = GetRightJoyStickCalibration(joycon_handle, true);
        break;
    default:
        break;
    }

    calibration.imu = GetImuCalibration(joycon_handle, true);
    return calibration;
}

void EnableImu(JoyconHandle& joycon_handle, bool enable) {
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const std::vector<u8> buffer{static_cast<u8>(enable ? 1 : 0)};
    SendSubCommand(joycon_handle, SubCommand::ENABLE_IMU, buffer, 1);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
}

void EnableRumble(JoyconHandle& joycon_handle, bool enable) {
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const std::vector<u8> buffer{static_cast<u8>(enable ? 1 : 0)};
    SendSubCommand(joycon_handle, SubCommand::ENABLE_VIBRATION, buffer, 1);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
}

void SendData(JoyconHandle& joycon_handle, std::span<const u8> buffer, std::size_t size) {
    SDL_hid_write(joycon_handle.handle, buffer.data(), size);
}

std::vector<u8> GetResponse(JoyconHandle& joycon_handle, SubCommand sc) {
    int tries = 0;
    std::vector<u8> buffer(max_resp_size);
    do {
        int result = SDL_hid_read_timeout(joycon_handle.handle, buffer.data(), max_resp_size, 100);
        if (result < 1) {
            LOG_ERROR(Input, "No response from joycon");
        }
        tries++;
    } while (tries < 10 && buffer[0] != 0x21 && buffer[14] != static_cast<u8>(sc));
    return buffer;
}

std::vector<u8> SendSubCommand(JoyconHandle& joycon_handle, SubCommand sc,
                               std::span<const u8> buffer, std::size_t size) {
    std::vector<u8> local_buffer(size + 11);

    local_buffer[0] = static_cast<u8>(Output::RUMBLE_AND_SUBCMD);
    local_buffer[1] = GetCounter(joycon_handle);
    for (std::size_t i = 0; i < 8; ++i) {
        local_buffer[i + 2] = default_buffer[i];
    }
    local_buffer[10] = static_cast<u8>(sc);
    for (std::size_t i = 0; i < size; ++i) {
        local_buffer[11 + i] = buffer[i];
    }

    SendData(joycon_handle, local_buffer, size + 11);
    return GetResponse(joycon_handle, sc);
}

std::vector<u8> ReadSPI(JoyconHandle& joycon_handle, CalAddr addr, u8 size) {
    std::vector<u8> buffer = {0x00, 0x00, 0x00, 0x00, size};
    std::vector<u8> local_buffer(size + 20);

    buffer[0] = static_cast<u8>(static_cast<u16>(addr) & 0x00FF);
    buffer[1] = static_cast<u8>((static_cast<u16>(addr) & 0xFF00) >> 8);
    for (std::size_t i = 0; i < 100; ++i) {
        local_buffer = SendSubCommand(joycon_handle, SubCommand::SPI_FLASH_READ, buffer, 5);
        if (local_buffer[15] == buffer[0] && local_buffer[16] == buffer[1]) {
            break;
        }
    }
    return std::vector<u8>(local_buffer.begin() + 20, local_buffer.end());
}

} // namespace InputCommon::Joycon
