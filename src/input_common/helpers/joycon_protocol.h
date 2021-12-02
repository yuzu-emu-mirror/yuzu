// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

// Based on dkms-hid-nintendo implementation and dekuNukem reverse engineering
// https://github.com/nicman23/dkms-hid-nintendo/blob/master/src/hid-nintendo.c
// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering

#pragma once

#include <span>
#include <vector>
#include <SDL_hidapi.h>

#include "common/common_types.h"

namespace InputCommon::Joycon {
constexpr u32 max_resp_size = 49;
constexpr std::array<u8, 8> default_buffer{0x0, 0x1, 0x40, 0x40, 0x0, 0x1, 0x40, 0x40};

enum class ControllerType {
    None,
    Left,
    Right,
    Pro,
};

enum class PadButton : u32 {
    Down = 0x000001,
    Up = 0x000002,
    Right = 0x000004,
    Left = 0x000008,
    LeftSR = 0x000010,
    LeftSL = 0x000020,
    L = 0x000040,
    ZL = 0x000080,
    Y = 0x000100,
    X = 0x000200,
    B = 0x000400,
    A = 0x000800,
    RightSL = 0x001000,
    RightSR = 0x002000,
    R = 0x004000,
    ZR = 0x008000,
    Minus = 0x010000,
    Plus = 0x020000,
    StickR = 0x040000,
    StickL = 0x080000,
    Home = 0x100000,
    Capture = 0x200000,
};

enum class OutputReport {
    RUMBLE_AND_SUBCMD = 0x01,
    FW_UPDATE_PKT = 0x03,
    RUMBLE_ONLY = 0x10,
    MCU_DATA = 0x11,
    USB_CMD = 0x80,
};

enum class InputReport {
    BUTTON_EVENT = 0x3F,
    SUBCMD_REPLY = 0x21,
    IMU_DATA = 0x30,
    MCU_DATA = 0x31,
    INPUT_USB_RESPONSE = 0x81,
};

enum class FeatureReport {
    Last_SUBCMD = 0x02,
    OTA_GW_UPGRADE = 0x70,
    SETUP_MEM_READ = 0x71,
    MEM_READ = 0x72,
    ERASE_MEM_SECTOR = 0x73,
    MEM_WRITE = 0x74,
    LAUNCH = 0x75,
};

enum class SubCommand {
    STATE = 0x00,
    MANUAL_BT_PAIRING = 0x01,
    REQ_DEV_INFO = 0x02,
    SET_REPORT_MODE = 0x03,
    TRIGGERS_ELAPSED = 0x04,
    GET_PAGE_LIST_STATE = 0x05,
    SET_HCI_STATE = 0x06,
    RESET_PAIRING_INFO = 0x07,
    LOW_POWER_MODE = 0x08,
    SPI_FLASH_READ = 0x10,
    SPI_FLASH_WRITE = 0x11,
    RESET_MCU = 0x20,
    SET_MCU_CONFIG = 0x21,
    SET_MCU_STATE = 0x22,
    SET_PLAYER_LIGHTS = 0x30,
    GET_PLAYER_LIGHTS = 0x31,
    SET_HOME_LIGHT = 0x38,
    ENABLE_IMU = 0x40,
    SET_IMU_SENSITIVITY = 0x41,
    WRITE_IMU_REG = 0x42,
    READ_IMU_REG = 0x43,
    ENABLE_VIBRATION = 0x48,
    GET_REGULATED_VOLTAGE = 0x50,
};

enum class UsbSubCommand {
    CONN_STATUS = 0x01,
    HADSHAKE = 0x02,
    BAUDRATE_3M = 0x03,
    NO_TIMEOUT = 0x04,
    EN_TIMEOUT = 0x05,
    RESET = 0x06,
    PRE_HANDSHAKE = 0x91,
    SEND_UART = 0x92,
};

enum class CalMagic {
    USR_MAGIC_0 = 0xB2,
    USR_MAGIC_1 = 0xA1,
    USRR_MAGI_SIZE = 2,
};

enum class CalAddr {
    USER_LEFT_MAGIC = 0X8010,
    USER_LEFT_DATA = 0X8012,
    USER_RIGHT_MAGIC = 0X801B,
    USER_RIGHT_DATA = 0X801D,
    USER_IMU_MAGIC = 0X8026,
    USER_IMU_DATA = 0X8028,
    SERIAL_NUMBER = 0X6000,
    DEVICE_TYPE = 0X6012,
    COLOR_EXIST = 0X601B,
    FACT_LEFT_DATA = 0X603d,
    FACT_RIGHT_DATA = 0X6046,
    COLOR_DATA = 0X6050,
    FACT_IMU_DATA = 0X6020,
};

enum class ReportMode {
    ACTIVE_POLLING_NFC_IR_CAMERA_DATA = 0x00,
    ACTIVE_POLLING_NFC_IR_CAMERA_CONFIGURATION = 0x01,
    ACTIVE_POLLING_NFC_IR_CAMERA_DATA_CONFIGURATION = 0x02,
    ACTIVE_POLLING_IR_CAMERA_DATA = 0x03,
    MCU_UPDATE_STATE = 0x23,
    STANDARD_FULL_60HZ = 0x30,
    NFC_IR_MODE_60HZ = 0x31,
    SIMPLE_HID_MODE = 0x3F,
};

enum class GyroSensitivity {
    DPS250,
    DPS500,
    DPS1000,
    DPS2000, // Default
};

enum class AccelerometerSensitivity {
    G8, // Default
    G4,
    G2,
    G16,
};

enum class GyroPerformance {
    HZ833,
    HZ208, // Default
};

enum class AccelerometerPerformance {
    HZ200,
    HZ100, // Default
};

struct ImuSensorCalibration {
    s16 offset;
    s16 scale;
};

struct ImuCalibration {
    std::array<ImuSensorCalibration, 3> accelerometer;
    std::array<ImuSensorCalibration, 3> gyro;
};

struct JoyStickAxisCalibration {
    u16 max;
    u16 min;
    u16 center;
};

struct JoyStickCalibration {
    JoyStickAxisCalibration x;
    JoyStickAxisCalibration y;
};

struct CalibrationData {
    JoyStickCalibration left_stick;
    JoyStickCalibration right_stick;
    ImuCalibration imu;
};

struct Color {
    u32 body;
    u32 buttons;
    u32 left_grip;
    u32 right_grip;
};

struct VibrationValue {
    f32 low_amplitude;
    f32 low_frequency;
    f32 high_amplitude;
    f32 high_frequency;
};

struct JoyconHandle {
    SDL_hid_device* handle;
    u8 packet_counter;
};

/**
 * Increments and returns the packet counter of the handle
 * @param joycon_handle device to send the data
 * @returns packet counter value
 */
u8 GetCounter(JoyconHandle& joycon_handle);

/**
 * Verifies and sets the joycon_handle if device is valid
 * @param joycon_handle device to send the data
 * @param device device info from the driver
 * @returns true if the device is valid
 */
bool CheckDeviceAccess(JoyconHandle& joycon_handle, SDL_hid_device_info* device);

/**
 * Sends a request to set the configuration of the motion sensor
 * @param joycon_handle device to send the data
 * @param gsen gyro sensitivity
 * @param gfrec gyro update frequency
 * @param asen accelerometer sensitivity
 * @param afrec accelerometer update frequency
 */
void SetImuConfig(JoyconHandle& joycon_handle, GyroSensitivity gsen, GyroPerformance gfrec,
                  AccelerometerSensitivity asen, AccelerometerPerformance afrec);

/**
 * Encondes the amplitude to be sended on a packet
 * @param amplification original amplification value
 * @returns encoded amplification value
 */
f32 EncodeRumbleAmplification(f32 amplification);

/**
 * Sends a request to set the vibration of the joycon
 * @param joycon_handle device to send the data
 * @param vibration amplitude and frequency of the vibration
 */
void SetVibration(JoyconHandle& joycon_handle, VibrationValue vibration);

/**
 * Sends a request to set the polling mode of the joycon
 * @param joycon_handle device to send the data
 * @param report_mode polling mode to be set
 */
void SetReportMode(JoyconHandle& joycon_handle, Joycon::ReportMode report_mode);

/**
 * Sends a request to set a specific led pattern
 * @param joycon_handle device to send the data
 * @param leds led pattern to be set
 */
void SetLedConfig(JoyconHandle& joycon_handle, u8 leds);

/**
 * Sends a request to obtain the joycon colors from memory
 * @param joycon_handle device to read the data
 * @returns color object with the colors of the joycon
 */
Color GetColor(JoyconHandle& joycon_handle);

/**
 * Sends a request to obtain the joycon mac address from handle
 * @param joycon_handle device to read the data
 * @returns array containing the mac address
 */
std::array<u8, 6> GetMacAddress(JoyconHandle& joycon_handle);

/**
 * Sends a request to obtain the joycon serial number from memory
 * @param joycon_handle device to read the data
 * @returns array containing the serial number
 */
std::array<u8, 15> GetSerialNumber(JoyconHandle& joycon_handle);

/**
 * Sends a request to obtain the joycon type from handle
 * @param joycon_handle device to read the data
 * @returns controller type of the joycon
 */
ControllerType GetDeviceType(JoyconHandle& joycon_handle);

/**
 * Sends a request to obtain the joycon tyoe from memory
 * @param joycon_handle device to read the data
 * @returns controller type of the joycon
 */
ControllerType GetDeviceType(SDL_hid_device_info* device_info);

/**
 * Sends a request to obtain the joycon firmware version
 * @param joycon_handle device to read the data
 * @returns u16 with the version number
 */
u16 GetVersionNumber(JoyconHandle& joycon_handle);

/**
 * Sends a request to obtain the left stick calibration from memory
 * @param joycon_handle device to read the data
 * @param is_factory_calibration if true factory values will be returned
 * @returns JoyStickCalibration of the left joystick
 */
JoyStickCalibration GetLeftJoyStickCalibration(JoyconHandle& joycon_handle,
                                               bool is_factory_calibration);

/**
 * Sends a request to obtain the right stick calibration from memory
 * @param joycon_handle device to read the data
 * @param is_factory_calibration if true factory values will be returned
 * @returns JoyStickCalibration of the left joystick
 */
JoyStickCalibration GetRightJoyStickCalibration(JoyconHandle& joycon_handle,
                                                bool is_factory_calibration);

/**
 * Sends a request to obtain the motion calibration from memory
 * @param joycon_handle device to read the data
 * @param is_factory_calibration if true factory values will be returned
 * @returns ImuCalibration of the joystick motion
 */
ImuCalibration GetImuCalibration(JoyconHandle& joycon_handle, bool is_factory_calibration);

/**
 * Requests user calibration from the joystick
 * @param joycon_handle device to read the data
 * @param controller_type type of calibration to be requested
 * @returns User CalibrationData of the joystick
 */
CalibrationData GetUserCalibrationData(JoyconHandle& joycon_handle, ControllerType controller_type);

/**
 * Requests factory calibration from the joystick
 * @param joycon_handle device to read the data
 * @param controller_type type of calibration to be requested
 * @returns Factory CalibrationData of the joystick
 */
CalibrationData GetFactoryCalibrationData(JoyconHandle& joycon_handle,
                                          ControllerType controller_type);

/**
 * Sends a request to enable motion
 * @param joycon_handle device to read the data
 * @param is_factory_calibration if true motion data will be enabled
 */
void EnableImu(JoyconHandle& joycon_handle, bool enable);

/**
 * Sends a request to enable vibrations
 * @param joycon_handle device to read the data
 * @param is_factory_calibration if true rumble will be enabled
 */
void EnableRumble(JoyconHandle& joycon_handle, bool enable);

/**
 * Sends data to the joycon device
 * @param joycon_handle device to send the data
 * @param buffer data to be send
 * @param size size in bytes of the buffer
 */
void SendData(JoyconHandle& joycon_handle, std::span<const u8> buffer, std::size_t size);

/**
 * Waits for incoming data of the joycon device that matchs the subcommand
 * @param joycon_handle device to read the data
 * @param sub_command type of data to be returned
 * @returns a buffer containing the responce
 */
std::vector<u8> GetResponse(JoyconHandle& joycon_handle, SubCommand sub_command);

/**
 * Sends data to the joycon device
 * @param joycon_handle device to send the data
 * @param buffer data to be send
 * @param size size in bytes of the buffer
 */
std::vector<u8> SendSubCommand(JoyconHandle& joycon_handle, SubCommand sc,
                               std::span<const u8> buffer, std::size_t size);

/**
 * Sends data to the joycon device
 * @param joycon_handle device to send the data
 * @param buffer data to be send
 * @param size size in bytes of the buffer
 */
std::vector<u8> ReadSPI(JoyconHandle& joycon_handle, CalAddr addr, u8 size);

} // namespace InputCommon::Joycon
