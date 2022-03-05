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

ErrorCode CheckDeviceAccess(JoyconHandle& joycon_handle, SDL_hid_device_info* device_info) {
    ControllerType controller_type{ControllerType::None};
    const auto result = GetDeviceType(device_info, controller_type);
    if (result != ErrorCode::Success || controller_type == ControllerType::None) {
        return ErrorCode::UnsupportedControllerType;
    }

    joycon_handle.handle =
        SDL_hid_open(device_info->vendor_id, device_info->product_id, device_info->serial_number);
    if (!joycon_handle.handle) {
        LOG_ERROR(Input, "Yuzu can't gain access to this device: ID {:04X}:{:04X}.",
                  device_info->vendor_id, device_info->product_id);
        return ErrorCode::HandleInUse;
    }
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return ErrorCode::Success;
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

ErrorCode SetVibration(JoyconHandle& joycon_handle, VibrationValue vibration) {
    std::vector<u8> buffer(Joycon::max_resp_size);

    buffer[0] = static_cast<u8>(Joycon::OutputReport::RUMBLE_ONLY);
    buffer[1] = GetCounter(joycon_handle);

    if (vibration.high_amplitude == 0.0f && vibration.low_amplitude == 0.0f) {
        for (std::size_t i = 0; i < Joycon::default_buffer.size(); ++i) {
            buffer[2 + i] = Joycon::default_buffer[i];
        }
        return SendData(joycon_handle, buffer, max_resp_size);
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

    return SendData(joycon_handle, buffer, max_resp_size);
}

ErrorCode SetImuConfig(JoyconHandle& joycon_handle, GyroSensitivity gsen, GyroPerformance gfrec,
                       AccelerometerSensitivity asen, AccelerometerPerformance afrec) {
    const std::vector<u8> buffer{static_cast<u8>(gsen), static_cast<u8>(asen),
                                 static_cast<u8>(gfrec), static_cast<u8>(afrec)};
    std::vector<u8> output;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const auto result = SendSubCommand(joycon_handle, SubCommand::SET_IMU_SENSITIVITY, buffer,
                                       buffer.size(), output);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return result;
}

ErrorCode SetLedConfig(JoyconHandle& joycon_handle, u8 leds) {
    const std::vector<u8> buffer{leds};
    std::vector<u8> output;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const auto result =
        SendSubCommand(joycon_handle, SubCommand::SET_PLAYER_LIGHTS, buffer, buffer.size(), output);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return result;
}

ErrorCode SetReportMode(JoyconHandle& joycon_handle, ReportMode report_mode) {
    const std::vector<u8> buffer{static_cast<u8>(report_mode)};
    std::vector<u8> output;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const auto result =
        SendSubCommand(joycon_handle, SubCommand::SET_REPORT_MODE, buffer, buffer.size(), output);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return result;
}

ErrorCode GetColor(JoyconHandle& joycon_handle, Color& color) {
    std::vector<u8> buffer;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const auto result = ReadSPI(joycon_handle, Joycon::CalAddr::COLOR_DATA, buffer, 12);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    if (result != ErrorCode::Success) {
        return result;
    }
    color.body = static_cast<u32>((buffer[2] << 16) | (buffer[1] << 8) | buffer[0]);
    color.buttons = static_cast<u32>((buffer[5] << 16) | (buffer[4] << 8) | buffer[3]);
    color.left_grip = static_cast<u32>((buffer[8] << 16) | (buffer[7] << 8) | buffer[6]);
    color.right_grip = static_cast<u32>((buffer[11] << 16) | (buffer[10] << 8) | buffer[9]);
    return ErrorCode::Success;
}

ErrorCode GetMacAddress(JoyconHandle& joycon_handle, MacAddress& mac_address) {
    wchar_t buffer[255];
    const auto result =
        SDL_hid_get_serial_number_string(joycon_handle.handle, buffer, std::size(buffer));
    if (result == -1) {
        LOG_ERROR(Input, "Unable to get mac address {}", result);
        return ErrorCode::ErrorReadingData;
    }
    for (std::size_t i = 0; i < mac_address.size(); ++i) {
        wchar_t value[3] = {buffer[i * 2], buffer[(i * 2) + 1]};
        mac_address[i] = static_cast<u8>(std::stoi(value, 0, 16));
    }
    return ErrorCode::Success;
}

ErrorCode GetSerialNumber(JoyconHandle& joycon_handle, SerialNumber& serial_number) {
    std::vector<u8> buffer;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const auto result = ReadSPI(joycon_handle, Joycon::CalAddr::SERIAL_NUMBER, buffer, 16);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    if (result != ErrorCode::Success) {
        return result;
    }
    for (std::size_t i = 0; i < serial_number.size(); ++i) {
        serial_number[i] = buffer[i + 1];
    }
    return ErrorCode::Success;
}

ErrorCode GetDeviceType(SDL_hid_device_info* device_info, ControllerType& controller_type) {
    constexpr u16 nintendo_vendor_id = 0x057e;
    constexpr u16 left_joycon_id = 0x2006;
    constexpr u16 right_joycon_id = 0x2007;
    constexpr u16 pro_controller_id = 0x2009;

    if (device_info->vendor_id != nintendo_vendor_id) {
        return ErrorCode::UnsupportedControllerType;
    }

    switch (device_info->product_id) {
    case left_joycon_id:
        controller_type = ControllerType::Left;
        break;
    case right_joycon_id:
        controller_type = ControllerType::Right;
        break;
    case pro_controller_id:
        controller_type = ControllerType::Pro;
        break;
    default:
        controller_type = ControllerType::None;
        break;
    }
    return ErrorCode::Success;
}

ErrorCode GetDeviceType(JoyconHandle& joycon_handle, ControllerType& controller_type) {
    std::vector<u8> buffer;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const auto result = ReadSPI(joycon_handle, Joycon::CalAddr::DEVICE_TYPE, buffer, 1);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    if (result != ErrorCode::Success) {
        return result;
    }
    controller_type = static_cast<Joycon::ControllerType>(buffer[0]);
    return ErrorCode::Success;
}

ErrorCode GetVersionNumber([[maybe_unused]] JoyconHandle& joycon_handle, FirmwareVersion& version) {
    // Not implemented
    version = FirmwareVersion::Rev_0;
    return ErrorCode::Success;
}

ErrorCode GetLeftJoyStickCalibration(JoyconHandle& joycon_handle, JoyStickCalibration& calibration,
                                     bool is_factory_calibration) {
    std::vector<u8> buffer;
    ErrorCode result{ErrorCode::Unknown};
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    if (is_factory_calibration) {
        result = ReadSPI(joycon_handle, CalAddr::FACT_LEFT_DATA, buffer, 9);
    } else {
        result = ReadSPI(joycon_handle, CalAddr::USER_LEFT_DATA, buffer, 9);
    }
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);

    if (result != ErrorCode::Success) {
        return result;
    }

    calibration.x.max = static_cast<u16>(((buffer[1] & 0x0F) << 8) | buffer[0]);
    calibration.y.max = static_cast<u16>((buffer[2] << 4) | (buffer[1] >> 4));
    calibration.x.center = static_cast<u16>(((buffer[4] & 0x0F) << 8) | buffer[3]);
    calibration.y.center = static_cast<u16>((buffer[5] << 4) | (buffer[4] >> 4));
    calibration.x.min = static_cast<u16>(((buffer[7] & 0x0F) << 8) | buffer[6]);
    calibration.y.min = static_cast<u16>((buffer[8] << 4) | (buffer[7] >> 4));

    // Nintendo fix for drifting stick
    // result = ReadSPI(0x60, 0x86 ,buffer, 16);
    // calibration.deadzone = (u16)((buffer[4] << 8) & 0xF00 | buffer[3]);
    return ErrorCode::Success;
}

ErrorCode GetRightJoyStickCalibration(JoyconHandle& joycon_handle, JoyStickCalibration& calibration,
                                      bool is_factory_calibration) {
    std::vector<u8> buffer;
    ErrorCode result{ErrorCode::Unknown};
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    if (is_factory_calibration) {
        result = ReadSPI(joycon_handle, CalAddr::FACT_RIGHT_DATA, buffer, 9);
    } else {
        result = ReadSPI(joycon_handle, CalAddr::USER_RIGHT_DATA, buffer, 9);
    }
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);

    if (result != ErrorCode::Success) {
        return result;
    }

    calibration.x.center = static_cast<u16>(((buffer[1] & 0x0F) << 8) | buffer[0]);
    calibration.y.center = static_cast<u16>((buffer[2] << 4) | (buffer[1] >> 4));
    calibration.x.min = static_cast<u16>(((buffer[4] & 0x0F) << 8) | buffer[3]);
    calibration.y.min = static_cast<u16>((buffer[5] << 4) | (buffer[4] >> 4));
    calibration.x.max = static_cast<u16>(((buffer[7] & 0x0F) << 8) | buffer[6]);
    calibration.y.max = static_cast<u16>((buffer[8] << 4) | (buffer[7] >> 4));

    // Nintendo fix for drifting stick
    // buffer = ReadSPI(0x60, 0x98 , 16);
    // joystick.deadzone = (u16)((buffer[4] << 8) & 0xF00 | buffer[3]);
    return ErrorCode::Success;
}

ErrorCode GetImuCalibration(JoyconHandle& joycon_handle, ImuCalibration& calibration,
                            bool is_factory_calibration) {
    std::vector<u8> buffer;
    ErrorCode result{ErrorCode::Unknown};
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    if (is_factory_calibration) {
        result = ReadSPI(joycon_handle, CalAddr::FACT_IMU_DATA, buffer, 24);
    } else {
        result = ReadSPI(joycon_handle, CalAddr::USER_IMU_DATA, buffer, 24);
    }
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);

    if (result != ErrorCode::Success) {
        return result;
    }

    for (std::size_t i = 0; i < calibration.accelerometer.size(); ++i) {
        const std::size_t index = i * 2;
        calibration.accelerometer[i].offset =
            static_cast<s16>(buffer[index] | (buffer[index + 1] << 8));
        calibration.accelerometer[i].scale =
            static_cast<s16>(buffer[index + 6] | (buffer[index + 7] << 8));
    }
    for (std::size_t i = 0; i < calibration.gyro.size(); ++i) {
        const std::size_t index = i * 2;
        calibration.gyro[i].offset =
            static_cast<s16>(buffer[index + 12] | (buffer[index + 13] << 8));
        calibration.gyro[i].scale =
            static_cast<s16>(buffer[index + 18] | (buffer[index + 19] << 8));
    }
    return ErrorCode::Success;
}

ErrorCode GetUserCalibrationData(JoyconHandle& joycon_handle, CalibrationData& calibration_data,
                                 ControllerType controller_type) {
    ErrorCode result{ErrorCode::Unknown};
    switch (controller_type) {
    case ControllerType::Left:
        result = GetLeftJoyStickCalibration(joycon_handle, calibration_data.left_stick, false);
        if (result != ErrorCode::Success) {
            return result;
        }
        break;
    case ControllerType::Right:
        result = GetRightJoyStickCalibration(joycon_handle, calibration_data.right_stick, false);
        if (result != ErrorCode::Success) {
            return result;
        }
        break;
    case Joycon::ControllerType::Pro:
        result = GetLeftJoyStickCalibration(joycon_handle, calibration_data.left_stick, false);
        if (result != ErrorCode::Success) {
            return result;
        }
        result = GetRightJoyStickCalibration(joycon_handle, calibration_data.right_stick, false);
        if (result != ErrorCode::Success) {
            return result;
        }
        break;
    default:
        return ErrorCode::UnsupportedControllerType;
    }

    result = GetImuCalibration(joycon_handle, calibration_data.imu, false);

    if (result != ErrorCode::Success) {
        return result;
    }

    return ErrorCode::Success;
}

ErrorCode GetFactoryCalibrationData(JoyconHandle& joycon_handle, CalibrationData& calibration_data,
                                    ControllerType controller_type) {
    ErrorCode result{ErrorCode::Unknown};
    switch (controller_type) {
    case ControllerType::Left:
        result = GetLeftJoyStickCalibration(joycon_handle, calibration_data.left_stick, true);
        if (result != ErrorCode::Success) {
            return result;
        }
        break;
    case ControllerType::Right:
        result = GetRightJoyStickCalibration(joycon_handle, calibration_data.right_stick, true);
        if (result != ErrorCode::Success) {
            return result;
        }
        break;
    case Joycon::ControllerType::Pro:
        result = GetLeftJoyStickCalibration(joycon_handle, calibration_data.left_stick, true);
        if (result != ErrorCode::Success) {
            return result;
        }
        result = GetRightJoyStickCalibration(joycon_handle, calibration_data.right_stick, true);
        if (result != ErrorCode::Success) {
            return result;
        }
        break;
    default:
        return ErrorCode::UnsupportedControllerType;
    }

    result = GetImuCalibration(joycon_handle, calibration_data.imu, true);

    if (result != ErrorCode::Success) {
        return result;
    }

    return ErrorCode::Success;
}

ErrorCode EnableImu(JoyconHandle& joycon_handle, bool enable) {
    const std::vector<u8> buffer{static_cast<u8>(enable ? 1 : 0)};
    std::vector<u8> output;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const auto result =
        SendSubCommand(joycon_handle, SubCommand::ENABLE_IMU, buffer, buffer.size(), output);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return result;
}

ErrorCode EnableRumble(JoyconHandle& joycon_handle, bool enable) {
    const std::vector<u8> buffer{static_cast<u8>(enable ? 1 : 0)};
    std::vector<u8> output;
    SDL_hid_set_nonblocking(joycon_handle.handle, 0);
    const auto result =
        SendSubCommand(joycon_handle, SubCommand::ENABLE_VIBRATION, buffer, buffer.size(), output);
    SDL_hid_set_nonblocking(joycon_handle.handle, 1);
    return result;
}

ErrorCode SendData(JoyconHandle& joycon_handle, std::span<const u8> buffer, std::size_t size) {
    const auto result = SDL_hid_write(joycon_handle.handle, buffer.data(), size);
    if (result == -1) {
        return ErrorCode::ErrorWritingData;
    }
    return ErrorCode::Success;
}

ErrorCode GetResponse(JoyconHandle& joycon_handle, SubCommand sc, std::vector<u8>& output) {
    constexpr int timeout_mili = 100;
    constexpr int max_tries = 10;
    int tries = 0;
    std::vector<u8> buffer(max_resp_size);
    do {
        int result =
            SDL_hid_read_timeout(joycon_handle.handle, buffer.data(), max_resp_size, timeout_mili);
        if (result < 1) {
            LOG_ERROR(Input, "No response from joycon");
        }
        tries++;
    } while (tries < max_tries && buffer[0] != 0x21 && buffer[14] != static_cast<u8>(sc));

    if (tries >= max_tries) {
        return ErrorCode::Timeout;
    }
    if (buffer[0] != 0x21 && buffer[14] != static_cast<u8>(sc)) {
        return ErrorCode::WrongReply;
    }

    output = buffer;
    return ErrorCode::Success;
}

ErrorCode SendSubCommand(JoyconHandle& joycon_handle, SubCommand sc, std::span<const u8> buffer,
                         std::size_t size, std::vector<u8>& output) {
    std::vector<u8> local_buffer(size + 11);

    local_buffer[0] = static_cast<u8>(OutputReport::RUMBLE_AND_SUBCMD);
    local_buffer[1] = GetCounter(joycon_handle);
    for (std::size_t i = 0; i < 8; ++i) {
        local_buffer[i + 2] = default_buffer[i];
    }
    local_buffer[10] = static_cast<u8>(sc);
    for (std::size_t i = 0; i < size; ++i) {
        local_buffer[11 + i] = buffer[i];
    }

    auto result = SendData(joycon_handle, local_buffer, size + 11);

    if (result != ErrorCode::Success) {
        return result;
    }

    result = GetResponse(joycon_handle, sc, output);

    return ErrorCode::Success;
}

ErrorCode ReadSPI(JoyconHandle& joycon_handle, CalAddr addr, std::vector<u8>& output, u8 size) {
    constexpr std::size_t max_tries = 10;
    std::size_t tries = 0;
    std::vector<u8> buffer = {0x00, 0x00, 0x00, 0x00, size};
    std::vector<u8> local_buffer(size + 20);

    buffer[0] = static_cast<u8>(static_cast<u16>(addr) & 0x00FF);
    buffer[1] = static_cast<u8>((static_cast<u16>(addr) & 0xFF00) >> 8);
    do {
        const auto result = SendSubCommand(joycon_handle, SubCommand::SPI_FLASH_READ, buffer,
                                           buffer.size(), local_buffer);
        if (result != ErrorCode::Success) {
            return result;
        }
        tries++;
    } while (tries < max_tries && (local_buffer[15] != buffer[0] || local_buffer[16] != buffer[1]));

    if (tries >= max_tries) {
        return ErrorCode::Timeout;
    }

    output = std::vector<u8>(local_buffer.begin() + 20, local_buffer.end());
    return ErrorCode::Success;
}

} // namespace InputCommon::Joycon
