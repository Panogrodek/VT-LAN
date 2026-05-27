#pragma once
#include "State Machine.hpp"
#include "graphics/Texture.hpp"
#include "managers/ChatManager.hpp"
#include "managers/FileTransferManager.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "audio/AudioVoiceReceiving.hpp"

// --------------------------------------------------------------------------
// Chat message
// --------------------------------------------------------------------------
struct ChatMessage {
	enum class Type { Text, Image, FileReceived };

	Type        type      = Type::Text;
	uint8_t     authorID  = 0;
	std::string authorLabel;

	// Text
	std::string text;

	// Image
	std::string imagePath;
	std::unique_ptr<fs::Texture> imageTexture;
	float imageW = 0.f;
	float imageH = 0.f;

	// FileReceived
	std::string savedPath;
	uint64_t    fileSize  = 0;

	ChatMessage() = default;
	ChatMessage(ChatMessage&&) = default;
	ChatMessage& operator=(ChatMessage&&) = default;
	ChatMessage(const ChatMessage&) = delete;
	ChatMessage& operator=(const ChatMessage&) = delete;
};

// --------------------------------------------------------------------------
class Lobby : public State {
public:
	// Session state shared from LoginScreen before entering Lobby
	static std::string s_localDisplayName; // display name of the local user
	static std::string s_roomPassword;      // host's password (empty = open room)
	static std::string s_joinPassword;      // password entered when joining
	static std::string s_hostIP;            // IP the host bound to
	static int         s_hostPort;          // port the host is on
	static bool        s_showInviteOnStart; // auto-open invite modal when Lobby is pushed by host

	Lobby();
	~Lobby() = default;

	void OnDropFile(const std::string& path) { m_pendingDropPath = path; }

private:
	virtual void Update() override;
	virtual void Render() override;

	void AnnounceSelf();          // broadcast own name + send auth on first connection
	void PollNetworkMessages();
	void UpdateVoiceConnections();
	void UpdateChat();
	void UpdateInviteModal();
	void AttachFile(const std::string& path);

	// Helpers
	std::string GetClientLabel(uint8_t gameID, bool isSelf, bool isHost) const;
	bool        TryLoadImageMessage(const std::string& path, ChatMessage& out);
	void        PostTextMessage(uint8_t authorID, const std::string& label, const std::string& text);
	void        PostFileReceivedMessage(const FileTransferManager::ReceivedFile& rf);

	// --- Chat ---
	std::vector<std::unique_ptr<ChatMessage>> m_messages;
	char        m_inputBuf[1024] = {};
	bool        m_scrollToBottom = false;
	std::string m_pendingDropPath;
	std::string m_lastSelectedFile;

	// File transfers in progress (transferID -> latest progress snapshot)
	std::unordered_map<uint32_t, FileTransferManager::TransferProgress> m_fileProgress;

	// --- Voice ---
	std::unordered_map<uint8_t, fs::PlayerVoiceSettings> m_voiceSettings;

	// --- Player names (gameID -> display name, received via \x01NAME: messages) ---
	std::unordered_map<uint8_t, std::string> m_playerNames;

	// --- Self-announcement flags ---
	bool   m_nameAnnounced      = false;
	bool   m_authSent           = false;
	size_t m_lastConnectionCount = 0; // for detecting new joins to re-announce name

	// --- Deferred disconnect (set inside message poll loop to avoid dangling refs) ---
	bool   m_pendingDisconnect  = false;

	// --- Invite popup state ---
	bool m_showInvitePopup = false;
	bool m_inviteCopied    = false;

	static constexpr float kLeftPanelWidth = 290.f;
	static constexpr float kMaxImageWidth  = 320.f;
	static constexpr float kMaxImageHeight = 240.f;
};
