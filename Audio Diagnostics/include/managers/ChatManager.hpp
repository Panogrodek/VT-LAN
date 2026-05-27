#pragma once
#include <string>
#include <deque>
#include <mutex>
#include <vector>
#include <cstdint>

class ChatManager {
public:
	struct Message {
		uint8_t     senderID;
		std::string text;
	};

	// Broadcast a text message to all connected players; shows locally on self without waiting for echo
	void Send(uint8_t selfID, const std::string& text);

	// Returns and clears all messages received from the network since the last call
	std::vector<Message> Poll();

	// Called from the RPC handler (runs on the main thread via the networking receive pass)
	void Enqueue(uint8_t senderID, const std::string& text);

private:
	std::mutex        m_mutex;
	std::deque<Message> m_pending;
};

extern ChatManager chatManager;
