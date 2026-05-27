#include "managers/ChatManager.hpp"
#include <networking/Networking.hpp>
#include <networking/RPC.hpp>

// Global instance - must be defined before the static registrar below so that the
// lambda can safely reference it when the RPC eventually fires.
ChatManager chatManager;

// File-scope static: constructor runs before main(), before getRPCMap().Lock() is called.
// Authority::All lets clients request a broadcast through the host, which then forwards
// it to everyone (including back to the sender, but we skip that echo below).
static fs::RPCRegistrar reg_chat_broadcast(
	"ChatBroadcast",
	fs::Authority::All,
	[](uint8_t senderID, std::string text) {
		// The host echoes the RPC back to the original sender; skip it to avoid
		// showing the message twice (we already show it locally on send).
		auto self = fs::connections.GetSelf();
		if (self.IsValid() && self.GetGameID() == senderID)
			return;
		chatManager.Enqueue(senderID, text);
	}
);

void ChatManager::Send(uint8_t selfID, const std::string& text) {
	if (!fs::networking.ActiveSession())
		return;

	const fs::RPC* rpc = fs::getRPCMap().GetRPC("ChatBroadcast");
	if (!rpc) return;

	if (fs::networking.Server()) {
		// Host sends directly to all connected players (also self-delivers via local queue)
		fs::sender.SendRPC(fs::AllCurrentReceivers(), rpc, selfID, text);
	}
	else {
		// Client asks the host to broadcast; receivers field tells host who to forward to
		fs::sender.SendRPCRequest(fs::AllCurrentReceivers(), rpc, selfID, text);
	}
}

std::vector<ChatManager::Message> ChatManager::Poll() {
	std::lock_guard<std::mutex> lk(m_mutex);
	std::vector<Message> out(m_pending.begin(), m_pending.end());
	m_pending.clear();
	return out;
}

void ChatManager::Enqueue(uint8_t senderID, const std::string& text) {
	std::lock_guard<std::mutex> lk(m_mutex);
	m_pending.push_back({ senderID, text });
}
