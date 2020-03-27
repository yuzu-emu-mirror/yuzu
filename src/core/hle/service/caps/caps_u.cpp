// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/caps/caps_u.h"

namespace Service::Capture {

class IAlbumAccessorApplicationSession final
    : public ServiceFramework<IAlbumAccessorApplicationSession> {
public:
    explicit IAlbumAccessorApplicationSession()
        : ServiceFramework{"IAlbumAccessorApplicationSession"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {2001, nullptr, "OpenAlbumMovieReadStream"},
            {2002, nullptr, "CloseAlbumMovieReadStream"},
            {2003, nullptr, "GetAlbumMovieReadStreamMovieDataSize"},
            {2004, nullptr, "ReadMovieDataFromAlbumMovieReadStream"},
            {2005, nullptr, "GetAlbumMovieReadStreamBrokenReason"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

CAPS_U::CAPS_U() : ServiceFramework("caps:u") {
    // clang-format off
    static const FunctionInfo functions[] = {
        {31, nullptr, "GetShimLibraryVersion"},
        {32, nullptr, "SetShimLibraryVersion"},
        {102, nullptr, "GetAlbumContentsFileListForApplication"},
        {103, nullptr, "DeleteAlbumContentsFileForApplication"},
        {104, nullptr, "GetAlbumContentsFileSizeForApplication"},
        {105, nullptr, "DeleteAlbumFileByAruidForDebug"},
        {110, nullptr, "LoadAlbumContentsFileScreenShotImageForApplication"},
        {120, nullptr, "LoadAlbumContentsFileThumbnailImageForApplication"},
        {130, nullptr, "PrecheckToCreateContentsForApplication"},
        {140, nullptr, "GetAlbumFileList1AafeAruidDeprecated"},
        {141, nullptr, "GetAlbumFileList2AafeUidAruidDeprecated"},
        {142, nullptr, "GetAlbumFileList3AaeAruid"},
        {143, nullptr, "GetAlbumFileList4AaeUidAruid"},
        {60002, nullptr, "OpenAccessorSessionForApplication"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

CAPS_U::~CAPS_U() = default;

} // namespace Service::Capture
