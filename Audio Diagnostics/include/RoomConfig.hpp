#pragma once
#include <string>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <utility>

#include "networking/ReceiverFlag.hpp"

namespace std {
    //template<>
    //inline struct hash<fs::ReceiverFlag> {
    //    size_t operator()(fs::ReceiverFlag f) const noexcept {
    //        return static_cast<size_t>(static_cast<uint16_t>(f));
    //    }
    //};
}

namespace RoomConfig {
    // Room creation settings
    inline std::string roomPassword;
    inline uint16_t    roomPort = 7777;

    // Pre-filled from command-line args (vtlan:// link)
    inline std::string prefilledIP;
    inline uint16_t    prefilledPort = 0;

    // Password entered by joining client
    inline std::string joinPassword;

    // Local IP detected at room creation time
    inline std::string localIP;

    // Authenticated player info queued by the ClientAuthenticate RPC function
    struct AuthInfo {
        std::string firstName;
        std::string lastName;
        fs::ReceiverFlag flag;
    };

    // Server-side auth queues (processed each frame in Application::Update)
    inline std::queue<fs::ReceiverFlag> authSenderQueue;  // filled by BindValidation
    inline std::queue<AuthInfo>         pendingAuth;       // filled by RPC function
    inline std::queue<fs::ReceiverFlag> pendingKicks;      // filled by RPC function on bad pw

    // Authenticated players map (flag -> firstName, lastName)
    inline std::unordered_map<fs::ReceiverFlag, std::pair<std::string, std::string>> authenticatedPlayers;

    // State flags
    inline bool kickedFromRoom = false;

    inline void ResetJoinState() {
        kickedFromRoom = false;
    }

    inline void ResetHostState() {
        while (!authSenderQueue.empty()) authSenderQueue.pop();
        while (!pendingAuth.empty())     pendingAuth.pop();
        while (!pendingKicks.empty())    pendingKicks.pop();
        authenticatedPlayers.clear();
    }
}
