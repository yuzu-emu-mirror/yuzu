// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <mutex>
#include <optional>
#include <tuple>

#include "common/param_package.h"
#include "core/frontend/input.h"
#include "core/settings.h"
#include "input_common/udp/client.h"
#include "input_common/udp/udp.h"

namespace InputCommon::CemuhookUDP {

class UDPTouchDevice final : public Input::TouchDevice {
public:
    explicit UDPTouchDevice(std::shared_ptr<DeviceStatus> status_) : status(std::move(status_)) {}
    std::tuple<float, float, bool> GetStatus() const override {
        std::lock_guard guard(status->update_mutex);
        return status->touch_status;
    }

private:
    std::shared_ptr<DeviceStatus> status;
};

class UDPMotionDevice final : public Input::MotionDevice {
public:
    explicit UDPMotionDevice(const std::string& host, int port, int pad_index, Common::Vec3f offset,
                             float sensitivity)
        : offset(offset), sensitivity(sensitivity) {
        status = std::make_shared<DeviceStatus>();
        client = std::make_unique<Client>(status, host, port, pad_index);
    }
    std::tuple<Common::Vec3<float>, Common::Vec3<float>> GetStatus() const override {
        std::lock_guard guard(status->update_mutex);
        std::tuple<Common::Vec3<float>, Common::Vec3<float>> motion = status->motion_status;
        std::get<0>(motion) = (std::get<0>(motion) + offset) * sensitivity;
        return motion;
    }

private:
    std::shared_ptr<DeviceStatus> status;
    std::unique_ptr<Client> client;
    Common::Vec3f offset;
    float sensitivity;
};

class UDPTouchFactory final : public Input::Factory<Input::TouchDevice> {
public:
    explicit UDPTouchFactory() {}

    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage& params) override {
        status = std::make_shared<DeviceStatus>();
        {
            std::lock_guard guard(status->update_mutex);
            status->touch_calibration = DeviceStatus::CalibrationData{};
            // These default values work well for DS4 but probably not other touch inputs
            status->touch_calibration->min_x = params.Get("min_x", 100);
            status->touch_calibration->min_y = params.Get("min_y", 50);
            status->touch_calibration->max_x = params.Get("max_x", 1800);
            status->touch_calibration->max_y = params.Get("max_y", 850);
        }
        std::string host = params.Get("host", "127.0.0.1");
        int port = params.Get("port", 26760);
        int pad_index = params.Get("pad_index", 0);
        client = std::make_shared<Client>(status, host, port, pad_index);
        return std::make_unique<UDPTouchDevice>(status);
    }

private:
    std::shared_ptr<DeviceStatus> status;
    std::shared_ptr<Client> client;
};

class UDPMotionFactory final : public Input::Factory<Input::MotionDevice> {
public:
    explicit UDPMotionFactory() {}

    std::unique_ptr<Input::MotionDevice> Create(const Common::ParamPackage& params) override {
        std::string host = params.Get("host", "127.0.0.1");
        int port = params.Get("port", 26760);
        int pad_index = params.Get("pad_index", 0);
        float cx = params.Get("cx", 0);
        float cy = params.Get("cy", 0);
        float cz = params.Get("cz", 0);
        Common::Vec3f offset(cx, cy, cz);
        float sensitivity = params.Get("sensitivity", 1);
        return std::make_unique<UDPMotionDevice>(host, port, pad_index, offset, sensitivity);
    }
};

State::State() {

    Input::RegisterFactory<Input::TouchDevice>("cemuhookudp", std::make_shared<UDPTouchFactory>());
    Input::RegisterFactory<Input::MotionDevice>("cemuhookudp",
                                                std::make_shared<UDPMotionFactory>());
}

State::~State() {
    Input::UnregisterFactory<Input::TouchDevice>("cemuhookudp");
    Input::UnregisterFactory<Input::MotionDevice>("cemuhookudp");
}

std::unique_ptr<State> Init() {
    return std::make_unique<State>();
}
} // namespace InputCommon::CemuhookUDP
