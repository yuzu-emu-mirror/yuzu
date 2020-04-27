// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Service::SM {
class ServiceManager;
}

namespace Core {
class System;
}

namespace Service::NIFM {

#pragma pack(1)
struct ClientId {
    u32 client_id{};
};
#pragma pack()
static_assert(sizeof(ClientId) == 0x4, "ClientId has incorrect size.");

#pragma pack(1)
struct Uuid {
    u128 uuid{};
};
#pragma pack()
static_assert(sizeof(Uuid) == 0x10, "Uuid has incorrect size.");

#pragma pack(1)
struct WirelessSettingData {
    u8 ssid_length{};
    std::array<u8, 0x21> ssid{};
    u8 unk1{};
    INSERT_PADDING_BYTES(0x1);
    u32 unk2{};
    u32 unk3{};
    std::array<u8, 0x41> passphrase{};
    INSERT_PADDING_BYTES(0x3);
};
#pragma pack()
static_assert(sizeof(WirelessSettingData) == 0x70, "WirelessSettingData has incorrect size.");

#pragma pack(1)
struct SfWirelessSettingData {
    u8 ssid_length{};
    std::array<u8, 0x20> ssid{};
    u8 unk1{};
    u8 unk2{};
    u8 unk3{};
    std::array<u8, 0x41> passphrase{};
};
#pragma pack()
static_assert(sizeof(SfWirelessSettingData) == 0x65, "SfWirelessSettingData has incorrect size.");

#pragma pack(1)
struct IpAddress {
    u32 address{};
};
#pragma pack()
static_assert(sizeof(IpAddress) == 0x4, "IpAddress has incorrect size.");

#pragma pack(1)
struct SubnetMask {
    u32 mask{};
};
#pragma pack()
static_assert(sizeof(SubnetMask) == 0x4, "SubnetMask has incorrect size.");

#pragma pack(1)
struct Gateway {
    u32 gateway{};
};
#pragma pack()
static_assert(sizeof(Gateway) == 0x4, "Gateway has incorrect size.");

#pragma pack(1)
struct IpAddressSetting {
    u8 is_automatic{};
    IpAddress ip_address{};
    SubnetMask subnet_mask{};
    Gateway gateway{};
};
#pragma pack()
static_assert(sizeof(IpAddressSetting) == 0xD, "IpAddressSetting has incorrect size.");

#pragma pack(1)
struct DNSSetting {
    u8 is_automatic{};
    u32 primary_dns{};
    u32 secondary_dns{};
};
#pragma pack()
static_assert(sizeof(DNSSetting) == 0x9, "DNSSetting has incorrect size.");

#pragma pack(1)
struct ProxySetting {
    u8 use_proxy{};
    INSERT_PADDING_BYTES(0x1);
    u16 port{};
    std::array<u8, 0x64> server{};
    u8 auto_authentication{};
    std::array<u8, 0x20> user_string{};
    std::array<u8, 0x20> password{};
    INSERT_PADDING_BYTES(0x1);
};
#pragma pack()
static_assert(sizeof(ProxySetting) == 0xAA, "ProxySetting has incorrect size.");

#pragma pack(1)
struct IpSettingData {
    IpAddressSetting address_settings{};
    DNSSetting dns_settings{};
    ProxySetting proxy_settings{};
    u16 mtu{};
};
#pragma pack()
static_assert(sizeof(IpSettingData) == 0xC2, "IpSettingData has incorrect size.");

#pragma pack(1)
struct NetworkProfileData {
    Uuid uuid{};
    std::array<u8, 0x40> network_name{};
    u32 unk1{};
    u32 unk2{};
    u8 unk3{};
    u8 unk4{};
    INSERT_PADDING_BYTES(0x2);
    WirelessSettingData wireless_data{};
    IpSettingData ip_data{};
};
#pragma pack()
static_assert(sizeof(NetworkProfileData) == 0x18E, "NetworkProfileData has incorrect size.");

#pragma pack(1)
struct SfNetworkProfileData {
    IpSettingData ip_data{};
    Uuid uuid{};
    std::array<u8, 0x40> network_name{};
    u8 unk1{};
    u8 unk2{};
    u8 unk3{};
    u8 unk4{};
    SfWirelessSettingData sf_wireless_data{};
    INSERT_PADDING_BYTES(0x1);
};
#pragma pack()
static_assert(sizeof(SfNetworkProfileData) == 0x17C, "SfNetworkProfileData has incorrect size.");

/// Registers all NIFM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::NIFM
