// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <queue>
#include "common/logging/log.h"
#include "common/uuid.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/friend/errors.h"
#include "core/hle/service/friend/friend.h"
#include "core/hle/service/friend/friend_interface.h"
#include "core/hle/service/kernel_helpers.h"

namespace Service::Friend {

class IFriendService final : public ServiceFramework<IFriendService> {
public:
    explicit IFriendService(Core::System& system_) : ServiceFramework{system_, "IFriendService"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetCompletionEvent"},
            {1, nullptr, "Cancel"},
            {10100, nullptr, "GetFriendListIds"},
            {10101, &IFriendService::GetFriendList, "GetFriendList"},
            {10102, nullptr, "UpdateFriendInfo"},
            {10110, nullptr, "GetFriendProfileImage"},
            {10120, nullptr, "Unknown10120"},
            {10121, nullptr, "Unknown10121"},
            {10200, nullptr, "SendFriendRequestForApplication"},
            {10211, nullptr, "AddFacedFriendRequestForApplication"},
            {10400, &IFriendService::GetBlockedUserListIds, "GetBlockedUserListIds"},
            {10420, nullptr, "Unknown10420"},
            {10421, nullptr, "Unknown10421"},
            {10500, nullptr, "GetProfileList"},
            {10600, nullptr, "DeclareOpenOnlinePlaySession"},
            {10601, &IFriendService::DeclareCloseOnlinePlaySession, "DeclareCloseOnlinePlaySession"},
            {10610, &IFriendService::UpdateUserPresence, "UpdateUserPresence"},
            {10700, &IFriendService::GetPlayHistoryRegistrationKey, "GetPlayHistoryRegistrationKey"},
            {10701, nullptr, "GetPlayHistoryRegistrationKeyWithNetworkServiceAccountId"},
            {10702, nullptr, "AddPlayHistory"},
            {11000, nullptr, "GetProfileImageUrl"},
            {20100, nullptr, "GetFriendCount"},
            {20101, nullptr, "GetNewlyFriendCount"},
            {20102, nullptr, "GetFriendDetailedInfo"},
            {20103, nullptr, "SyncFriendList"},
            {20104, nullptr, "RequestSyncFriendList"},
            {20110, nullptr, "LoadFriendSetting"},
            {20200, nullptr, "GetReceivedFriendRequestCount"},
            {20201, nullptr, "GetFriendRequestList"},
            {20300, nullptr, "GetFriendCandidateList"},
            {20301, nullptr, "GetNintendoNetworkIdInfo"},
            {20302, nullptr, "GetSnsAccountLinkage"},
            {20303, nullptr, "GetSnsAccountProfile"},
            {20304, nullptr, "GetSnsAccountFriendList"},
            {20400, nullptr, "GetBlockedUserList"},
            {20401, nullptr, "SyncBlockedUserList"},
            {20500, nullptr, "GetProfileExtraList"},
            {20501, nullptr, "GetRelationship"},
            {20600, nullptr, "GetUserPresenceView"},
            {20700, nullptr, "GetPlayHistoryList"},
            {20701, nullptr, "GetPlayHistoryStatistics"},
            {20800, nullptr, "LoadUserSetting"},
            {20801, nullptr, "SyncUserSetting"},
            {20900, nullptr, "RequestListSummaryOverlayNotification"},
            {21000, nullptr, "GetExternalApplicationCatalog"},
            {22000, nullptr, "GetReceivedFriendInvitationList"},
            {22001, nullptr, "GetReceivedFriendInvitationDetailedInfo"},
            {22010, nullptr, "GetReceivedFriendInvitationCountCache"},
            {30100, nullptr, "DropFriendNewlyFlags"},
            {30101, nullptr, "DeleteFriend"},
            {30110, nullptr, "DropFriendNewlyFlag"},
            {30120, nullptr, "ChangeFriendFavoriteFlag"},
            {30121, nullptr, "ChangeFriendOnlineNotificationFlag"},
            {30200, nullptr, "SendFriendRequest"},
            {30201, nullptr, "SendFriendRequestWithApplicationInfo"},
            {30202, nullptr, "CancelFriendRequest"},
            {30203, nullptr, "AcceptFriendRequest"},
            {30204, nullptr, "RejectFriendRequest"},
            {30205, nullptr, "ReadFriendRequest"},
            {30210, nullptr, "GetFacedFriendRequestRegistrationKey"},
            {30211, nullptr, "AddFacedFriendRequest"},
            {30212, nullptr, "CancelFacedFriendRequest"},
            {30213, nullptr, "GetFacedFriendRequestProfileImage"},
            {30214, nullptr, "GetFacedFriendRequestProfileImageFromPath"},
            {30215, nullptr, "SendFriendRequestWithExternalApplicationCatalogId"},
            {30216, nullptr, "ResendFacedFriendRequest"},
            {30217, nullptr, "SendFriendRequestWithNintendoNetworkIdInfo"},
            {30300, nullptr, "GetSnsAccountLinkPageUrl"},
            {30301, nullptr, "UnlinkSnsAccount"},
            {30400, nullptr, "BlockUser"},
            {30401, nullptr, "BlockUserWithApplicationInfo"},
            {30402, nullptr, "UnblockUser"},
            {30500, nullptr, "GetProfileExtraFromFriendCode"},
            {30700, nullptr, "DeletePlayHistory"},
            {30810, nullptr, "ChangePresencePermission"},
            {30811, nullptr, "ChangeFriendRequestReception"},
            {30812, nullptr, "ChangePlayLogPermission"},
            {30820, nullptr, "IssueFriendCode"},
            {30830, nullptr, "ClearPlayLog"},
            {30900, nullptr, "SendFriendInvitation"},
            {30910, nullptr, "ReadFriendInvitation"},
            {30911, nullptr, "ReadAllFriendInvitations"},
            {40100, nullptr, "Unknown40100"},
            {40400, nullptr, "Unknown40400"},
            {49900, nullptr, "DeleteNetworkServiceAccountCache"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    enum class PresenceFilter : u32 {
        None = 0,
        Online = 1,
        OnlinePlay = 2,
        OnlineOrOnlinePlay = 3,
    };

    struct SizedFriendFilter {
        PresenceFilter presence;
        u8 is_favorite;
        u8 same_app;
        u8 same_app_played;
        u8 arbitary_app_played;
        u64 group_id;
    };
    static_assert(sizeof(SizedFriendFilter) == 0x10, "SizedFriendFilter is an invalid size");

    void GetBlockedUserListIds(Kernel::HLERequestContext& ctx) {
        // This is safe to stub, as there should be no adverse consequences from reporting no
        // blocked users.
        LOG_WARNING(Service_Friend, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(0); // Indicates there are no blocked users
    }

    void DeclareCloseOnlinePlaySession(Kernel::HLERequestContext& ctx) {
        // Stub used by Splatoon 2
        LOG_WARNING(Service_Friend, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void UpdateUserPresence(Kernel::HLERequestContext& ctx) {
        // Stub used by Retro City Rampage
        LOG_WARNING(Service_Friend, "(STUBBED) called");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetPlayHistoryRegistrationKey(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto local_play = rp.Pop<bool>();
        const auto uuid = rp.PopRaw<Common::UUID>();

        LOG_WARNING(Service_Friend, "(STUBBED) called, local_play={}, uuid=0x{}", local_play,
                    uuid.Format());

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetFriendList(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto friend_offset = rp.Pop<u32>();
        const auto uuid = rp.PopRaw<Common::UUID>();
        [[maybe_unused]] const auto filter = rp.PopRaw<SizedFriendFilter>();
        const auto pid = rp.Pop<u64>();
        LOG_WARNING(Service_Friend, "(STUBBED) called, offset={}, uuid=0x{}, pid={}", friend_offset,
                    uuid.Format(), pid);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);

        rb.Push<u32>(0); // Friend count
        // TODO(ogniK): Return a buffer of u64s which are the "NetworkServiceAccountId"
    }
};

class INotificationService final : public ServiceFramework<INotificationService> {
public:
    explicit INotificationService(Core::System& system_, Common::UUID uuid_)
        : ServiceFramework{system_, "INotificationService"}, uuid{uuid_},
          service_context{system_, "INotificationService"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &INotificationService::GetEvent, "GetEvent"},
            {1, &INotificationService::Clear, "Clear"},
            {2, &INotificationService::Pop, "Pop"}
        };
        // clang-format on

        RegisterHandlers(functions);

        notification_event = service_context.CreateEvent("INotificationService:NotifyEvent");
    }

    ~INotificationService() override {
        service_context.CloseEvent(notification_event);
    }

private:
    void GetEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Friend, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(notification_event->GetReadableEvent());
    }

    void Clear(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Friend, "called");
        while (!notifications.empty()) {
            notifications.pop();
        }
        std::memset(&states, 0, sizeof(States));

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Pop(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Friend, "called");

        if (notifications.empty()) {
            LOG_ERROR(Service_Friend, "No notifications in queue!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERR_NO_NOTIFICATIONS);
            return;
        }

        const auto notification = notifications.front();
        notifications.pop();

        switch (notification.notification_type) {
        case NotificationTypes::HasUpdatedFriendsList:
            states.has_updated_friends = false;
            break;
        case NotificationTypes::HasReceivedFriendRequest:
            states.has_received_friend_request = false;
            break;
        default:
            // HOS seems not have an error case for an unknown notification
            LOG_WARNING(Service_Friend, "Unknown notification {:08X}",
                        notification.notification_type);
            break;
        }

        IPC::ResponseBuilder rb{ctx, 6};
        rb.Push(ResultSuccess);
        rb.PushRaw<SizedNotificationInfo>(notification);
    }

    enum class NotificationTypes : u32 {
        HasUpdatedFriendsList = 0x65,
        HasReceivedFriendRequest = 0x1
    };

    struct SizedNotificationInfo {
        NotificationTypes notification_type;
        INSERT_PADDING_WORDS(
            1); // TODO(ogniK): This doesn't seem to be used within any IPC returns as of now
        u64_le account_id;
    };
    static_assert(sizeof(SizedNotificationInfo) == 0x10,
                  "SizedNotificationInfo is an incorrect size");

    struct States {
        bool has_updated_friends;
        bool has_received_friend_request;
    };

    Common::UUID uuid;
    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* notification_event;
    std::queue<SizedNotificationInfo> notifications;
    States states{};
};

void Module::Interface::CreateFriendService(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IFriendService>(system);
    LOG_DEBUG(Service_Friend, "called");
}

void Module::Interface::CreateNotificationService(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    auto uuid = rp.PopRaw<Common::UUID>();

    LOG_DEBUG(Service_Friend, "called, uuid=0x{}", uuid.Format());

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<INotificationService>(system, uuid);
}

Module::Interface::Interface(std::shared_ptr<Module> module_, Core::System& system_,
                             const char* name)
    : ServiceFramework{system_, name}, module{std::move(module_)} {}

Module::Interface::~Interface() = default;

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    auto module = std::make_shared<Module>();
    std::make_shared<Friend>(module, system, "friend:a")->InstallAsService(service_manager);
    std::make_shared<Friend>(module, system, "friend:m")->InstallAsService(service_manager);
    std::make_shared<Friend>(module, system, "friend:s")->InstallAsService(service_manager);
    std::make_shared<Friend>(module, system, "friend:u")->InstallAsService(service_manager);
    std::make_shared<Friend>(module, system, "friend:v")->InstallAsService(service_manager);
}

} // namespace Service::Friend
