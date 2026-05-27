#include "states/Lobby.hpp"

#include <imgui/imgui.h>
#include <audio/AudioVoiceReceiving.hpp>
#include <audio/AudioVoiceCapturing.hpp>
#include <networking/Networking.hpp>
#include <imgui/ImGuiFileDialog.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <cstring>
#include <cstdio>
#include <string>

// Networking headers (included via Lobby.hpp) already pull in windows.h via Steam.
#ifdef _WIN32
#include <shellapi.h>
#endif

// --------------------------------------------------------------------------
// Static member definitions
// --------------------------------------------------------------------------
std::string Lobby::s_localDisplayName;
std::string Lobby::s_roomPassword;
std::string Lobby::s_joinPassword;
std::string Lobby::s_hostIP;
int         Lobby::s_hostPort         = 0;
bool        Lobby::s_showInviteOnStart = false;

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------
static bool IsImageExtension(const std::string& path)
{
	std::string ext = std::filesystem::path(path).extension().string();
	for (auto& c : ext) c = (char)tolower((unsigned char)c);
	return ext == ".png" || ext == ".jpg" || ext == ".jpeg"
	    || ext == ".bmp" || ext == ".gif" || ext == ".tga";
}

static std::pair<uint8_t, std::string> FindSelf(
	const decltype(fs::connections.GetAll())& all,
	std::function<std::string(uint8_t, bool, bool)> labelFn)
{
	for (const auto& [id, conn] : all)
		if (conn.IsSelf())
			return { id, labelFn(id, true, conn.IsHost()) };
	return { 0, "Klient 0" };
}

static std::string FormatBytes(uint64_t bytes)
{
	char buf[64];
	if (bytes < 1024)
		snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
	else if (bytes < 1024 * 1024)
		snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
	else if (bytes < 1024ull * 1024 * 1024)
		snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
	else
		snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
	return buf;
}

// Portable starts_with for C++17
static bool StartsWith(const std::string& s, const char* prefix)
{
	return s.rfind(prefix, 0) == 0;
}

// --------------------------------------------------------------------------
// Clipboard helper (used for invite popup)
// --------------------------------------------------------------------------
static void CopyTextToClipboard(const std::string& text)
{
#ifdef _WIN32
	if (!OpenClipboard(nullptr)) return;
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
	if (hg) {
		memcpy(GlobalLock(hg), text.c_str(), text.size() + 1);
		GlobalUnlock(hg);
		SetClipboardData(CF_TEXT, hg);
	}
	CloseClipboard();
#endif
}

static void OpenEmailInviteFromLobby(const std::string& ip, int port)
{
#ifdef _WIN32
	std::string link = "vtlan://" + ip + ":" + std::to_string(port);
	std::string portStr = std::to_string(port);
	std::string body =
		"Zaproszenie do spotkania prowadzonego w srodowisku VT-LAN\n\n"
		"Kliknij ponizszy link, aby dolaczyc:\n"
		+ link + "\n\n"
		"Jesli link nie zadziala, otworz aplikacje VT-LAN z nastepujacymi parametrami:\n"
		"Adres IP: " + ip + "\nPort: " + portStr;

	auto encode = [](const std::string& s) {
		std::string out;
		for (unsigned char c : s) {
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
				out += (char)c;
			} else if (c == '\n') {
				out += "%0A";
			} else {
				char h[4]; snprintf(h, sizeof(h), "%%%02X", c); out += h;
			}
		}
		return out;
	};

	// Gmail compose URL — opens in the browser, no mail client required.
	// Format: https://mail.google.com/mail/?view=cm&fs=1&su=SUBJECT&body=BODY
	std::string subject = "Zaproszenie do spotkania w VT-LAN";
	std::string gmailUrl =
		"https://mail.google.com/mail/?view=cm&fs=1&su=" + encode(subject) +
		"&body=" + encode(body);
	ShellExecuteA(nullptr, "open", gmailUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

// --------------------------------------------------------------------------
Lobby::Lobby()
{
	if (s_showInviteOnStart) {
		m_showInvitePopup  = true;
		s_showInviteOnStart = false;
	}
}

void Lobby::Update()
{
	AnnounceSelf();
	PollNetworkMessages();
	UpdateVoiceConnections();
	UpdateChat();
}

void Lobby::Render() {}

// --------------------------------------------------------------------------
// AnnounceSelf - sends name and auth once the connection is established
// --------------------------------------------------------------------------
void Lobby::AnnounceSelf()
{
	const auto& all = fs::connections.GetAll();

	uint8_t selfID = 0;
	for (const auto& [id, conn] : all)
		if (conn.IsSelf()) { selfID = id; break; }

	if (selfID == 0) return;

	// Always make sure our own name is set locally
	if (m_playerNames.find(selfID) == m_playerNames.end() && !s_localDisplayName.empty())
		m_playerNames[selfID] = s_localDisplayName;

	// Detect when a new player joins so existing players re-announce their names.
	// This ensures new joiners learn about everyone already in the room.
	size_t currentCount = all.size();
	bool newPlayerJoined = currentCount > m_lastConnectionCount;
	m_lastConnectionCount = currentCount;

	if (!m_nameAnnounced) {
		chatManager.Send(selfID, "\x01NAME:" + s_localDisplayName);
		m_nameAnnounced = true;
	} else if (newPlayerJoined) {
		chatManager.Send(selfID, "\x01NAME:" + s_localDisplayName);
	}

	if (!m_authSent && !fs::networking.Server()) {
		// Clients always send their password (empty string = no password)
		chatManager.Send(selfID, "\x01\AUTH:" + s_joinPassword);
		m_authSent = true;
	}
}

// --------------------------------------------------------------------------
// Network polling
// --------------------------------------------------------------------------
void Lobby::PollNetworkMessages()
{
	fileTransferManager.Tick();

	const auto& all = fs::connections.GetAll();

	for (auto& msg : chatManager.Poll()) {

		// --- Control messages (not displayed in chat) ---
		if (StartsWith(msg.text, "\x01NAME:")) {
			std::string name = msg.text.substr(6);
			if (!name.empty())
				m_playerNames[msg.senderID] = name;
			continue;
		}

		if (StartsWith(msg.text, "\x01\AUTH:")) {
			if (fs::networking.Server()) {
				// Host validates the client's password
				std::string clientPwd = msg.text.substr(6);
				bool ok = s_roomPassword.empty() || (clientPwd == s_roomPassword);
				if (!ok) {
					uint8_t selfID = 0;
					for (const auto& [id, conn] : all)
						if (conn.IsSelf()) { selfID = id; break; }
					// Broadcast rejection — targeted client checks its own ID
					chatManager.Send(selfID,
						"\x01\AUTH_FAIL:" + std::to_string(msg.senderID));
				}
			}
			continue;
		}

		if (StartsWith(msg.text, "\x01\AUTH_FAIL:")) {
			try {
				int targetID = std::stoi(msg.text.substr(11));
				for (const auto& [id, conn] : all) {
					if (conn.IsSelf() && (int)id == targetID) {
						// Defer disconnect — calling it here would destroy 'all'
						// and any subsequent message iteration would read freed memory.
						m_pendingDisconnect = true;
						break;
					}
				}
			} catch (...) {}
			continue;
		}

		// --- Regular text message ---
		auto it = all.find(msg.senderID);
		bool isHost = (it != all.end()) && it->second.IsHost();
		PostTextMessage(msg.senderID,
		                GetClientLabel(msg.senderID, false, isHost),
		                msg.text);
	}

	// Drain deferred disconnect — must happen after the poll loop so 'all' is no longer
	// in scope and connections.Destroy() cannot invalidate a live reference.
	if (m_pendingDisconnect) {
		m_pendingDisconnect = false;
		fs::networking.DisconnectFromServer();
		return; // connections are gone, nothing else to process this frame
	}

	for (auto& rf : fileTransferManager.PollCompleted())
		PostFileReceivedMessage(rf);

	for (auto& prog : fileTransferManager.PollProgress()) {
		if (prog.chunksReceived < prog.totalChunks)
			m_fileProgress[prog.transferID] = prog;
		else
			m_fileProgress.erase(prog.transferID);
	}
}

// --------------------------------------------------------------------------
std::string Lobby::GetClientLabel(uint8_t gameID, bool isSelf, bool isHost) const
{
	std::string name;
	auto it = m_playerNames.find(gameID);
	if (it != m_playerNames.end() && !it->second.empty())
		name = it->second;
	else
		name = "Klient " + std::to_string(gameID);

	if (isSelf) name += " (Ty)";
	if (isHost) name += " [host]";
	return name;
}

bool Lobby::TryLoadImageMessage(const std::string& path, ChatMessage& out)
{
	if (!IsImageExtension(path)) return false;

	out.imageTexture = std::make_unique<fs::Texture>();
	if (!out.imageTexture->LoadFromFile(path)) {
		out.imageTexture.reset();
		return false;
	}

	glm::vec2 sz = out.imageTexture->GetSize();
	float scaleW = (sz.x > kMaxImageWidth)  ? kMaxImageWidth  / sz.x : 1.f;
	float scaleH = (sz.y > kMaxImageHeight) ? kMaxImageHeight / sz.y : 1.f;
	float scale  = std::min(scaleW, scaleH);
	out.imageW = sz.x * scale;
	out.imageH = sz.y * scale;
	return true;
}

void Lobby::PostTextMessage(uint8_t authorID, const std::string& label, const std::string& text)
{
	auto msg         = std::make_unique<ChatMessage>();
	msg->type        = ChatMessage::Type::Text;
	msg->authorID    = authorID;
	msg->authorLabel = label;
	msg->text        = text;
	m_messages.push_back(std::move(msg));
	m_scrollToBottom = true;
}

void Lobby::PostFileReceivedMessage(const FileTransferManager::ReceivedFile& rf)
{
	const auto& all = fs::connections.GetAll();
	auto it = all.find(rf.senderID);
	bool isHost = (it != all.end()) && it->second.IsHost();
	std::string label = GetClientLabel(rf.senderID, false, isHost);

	if (IsImageExtension(rf.savedPath)) {
		auto msg         = std::make_unique<ChatMessage>();
		msg->type        = ChatMessage::Type::Image;
		msg->authorID    = rf.senderID;
		msg->authorLabel = label;
		msg->imagePath   = rf.savedPath;
		if (TryLoadImageMessage(rf.savedPath, *msg)) {
			m_messages.push_back(std::move(msg));
			m_scrollToBottom = true;
			return;
		}
	}

	auto msg         = std::make_unique<ChatMessage>();
	msg->type        = ChatMessage::Type::FileReceived;
	msg->authorID    = rf.senderID;
	msg->authorLabel = label;
	msg->text        = rf.filename;
	msg->savedPath   = rf.savedPath;
	msg->fileSize    = rf.totalSize;
	m_messages.push_back(std::move(msg));
	m_scrollToBottom = true;
}

void Lobby::AttachFile(const std::string& path)
{
	const auto& all = fs::connections.GetAll();
	auto [selfID, selfLabel] = FindSelf(all,
		[this](uint8_t id, bool s, bool h) { return GetClientLabel(id, s, h); });

	if (IsImageExtension(path)) {
		auto msg         = std::make_unique<ChatMessage>();
		msg->type        = ChatMessage::Type::Image;
		msg->authorID    = selfID;
		msg->authorLabel = selfLabel;
		msg->imagePath   = path;
		if (TryLoadImageMessage(path, *msg))
			m_messages.push_back(std::move(msg));
		else
			PostTextMessage(selfID, selfLabel, "[Blad ladowania obrazka: " + path + "]");
	} else {
		std::string name = std::filesystem::path(path).filename().string();
		auto msg         = std::make_unique<ChatMessage>();
		msg->type        = ChatMessage::Type::FileSent;
		msg->authorID    = selfID;
		msg->authorLabel = selfLabel;
		msg->text        = name;      // filename for display
		msg->savedPath   = path;      // original absolute path for click-to-open
		m_messages.push_back(std::move(msg));
	}

	m_scrollToBottom = true;

	if (fs::networking.ActiveSession())
		fileTransferManager.Send(selfID, path);
}

// --------------------------------------------------------------------------
// Invite modal (accessible from voice panel when hosting)
// --------------------------------------------------------------------------
void Lobby::UpdateInviteModal()
{
	if (!m_showInvitePopup) return;

	ImGui::OpenPopup("Zaproszenie##invite_modal");

	ImVec2 modalSize(440.f, 290.f);
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(
		{ (io.DisplaySize.x - modalSize.x) * 0.5f,
		  (io.DisplaySize.y - modalSize.y) * 0.5f },
		ImGuiCond_Always);
	ImGui::SetNextWindowSize(modalSize, ImGuiCond_Always);

	if (ImGui::BeginPopupModal("Zaproszenie##invite_modal", nullptr,
	    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.30f, 1.f, 1.f));
		ImGui::SetWindowFontScale(1.1f);
		ImGui::TextUnformatted("Zaproś innych uczestników");
		ImGui::SetWindowFontScale(1.f);
		ImGui::PopStyleColor();
		ImGui::Separator(); ImGui::Spacing();

		const float lw = 110.f;
		ImGui::Text("Adres IP:");
		ImGui::SameLine(lw);
		ImGui::TextColored(ImVec4(1.f, 1.f, 0.6f, 1.f), "%s", s_hostIP.c_str());

		ImGui::Text("Port:");
		ImGui::SameLine(lw);
		ImGui::TextColored(ImVec4(1.f, 1.f, 0.6f, 1.f), "%d", s_hostPort);

		std::string link = "vtlan://" + s_hostIP + ":" + std::to_string(s_hostPort);
		ImGui::Text("Link:");
		ImGui::SameLine(lw);
		ImGui::TextColored(ImVec4(0.55f, 0.80f, 1.f, 1.f), "%s", link.c_str());

		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		const float bW = (modalSize.x - 32.f - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

		std::string inviteText =
			"Otrzymano zaproszenie do spotkania prowadzonego w VT-LAN.\n"
			"Kliknij link: " + link + "\n"
			"lub wpisz: IP=" + s_hostIP + " Port=" + std::to_string(s_hostPort);

		if (ImGui::Button("Kopiuj zaproszenie", ImVec2(bW, 34.f))) {
			CopyTextToClipboard(inviteText);
			m_inviteCopied = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Wyslij przez Gmail", ImVec2(bW, 34.f)))
			OpenEmailInviteFromLobby(s_hostIP, s_hostPort);

		if (m_inviteCopied)
			ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "  Skopiowano do schowka!");

		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		if (ImGui::Button("Zamknij", ImVec2(modalSize.x - 32.f, 32.f))) {
			m_showInvitePopup = false;
			m_inviteCopied = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

// --------------------------------------------------------------------------
// Voice Connections - left panel
// --------------------------------------------------------------------------
void Lobby::UpdateVoiceConnections()
{
	ImGuiViewport* vp = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(kLeftPanelWidth, vp->WorkSize.y), ImGuiCond_Always);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.12f, 0.97f));
	ImGui::Begin("##VoicePanel", nullptr, flags);
	ImGui::PopStyleColor();

	// Header
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.30f, 1.f, 1.f));
	ImGui::SetWindowFontScale(1.05f);
	ImGui::TextUnformatted("UCZESTNICY");
	ImGui::SetWindowFontScale(1.f);
	ImGui::PopStyleColor();
	ImGui::Separator();
	ImGui::Spacing();

	const auto& all = fs::connections.GetAll();

	if (all.empty()) {
		ImGui::TextDisabled("Brak aktywnych połączeń...");
	} else {
		// Sync voice settings map with current connections
		for (const auto& [gameID, conn] : all)
			if (m_voiceSettings.find(gameID) == m_voiceSettings.end()) {
				fs::PlayerVoiceSettings def; def.gain = 1.f; def.muted = false;
				m_voiceSettings[gameID] = def;
			}
		for (auto it = m_voiceSettings.begin(); it != m_voiceSettings.end(); )
			it = all.find(it->first) == all.end() ? m_voiceSettings.erase(it) : ++it;

		// Reserve space for bottom controls
		const float kBottomH = 1.f
			+ ImGui::GetStyle().ItemSpacing.y * 6.f
			+ ImGui::GetFrameHeight() * 2.f
			+ 30.f + 4.f;

		ImGui::BeginChild("##VoiceList", ImVec2(0, -kBottomH), false);

		for (const auto& [gameID, conn] : all) {
			bool isSelf = conn.IsSelf();
			bool isHost = conn.IsHost();
			ImGui::PushID(gameID);

			// Player name with color
			ImVec4 nameCol = isSelf
				? ImVec4(0.55f, 0.85f, 1.f, 1.f)   // self = blue-ish
				: ImVec4(0.90f, 0.90f, 0.90f, 1.f); // others = white

			ImGui::TextColored(nameCol, "%s", GetClientLabel(gameID, isSelf, isHost).c_str());

			fs::PlayerVoiceSettings& cfg = m_voiceSettings[gameID];

			if (isSelf) {
				ImGui::SameLine();
				ImGui::TextDisabled("(mikrofon)");
			} else {
				const char* muteLabel = cfg.muted ? "Odcisz" : "Wycisz";
				if (ImGui::Button(muteLabel, ImVec2(64, 0))) cfg.muted = !cfg.muted;
				ImGui::SameLine();
				if (cfg.muted) ImGui::BeginDisabled();
				ImGui::SetNextItemWidth(78.f);
				ImGui::SliderFloat("##gain", &cfg.gain, 0.f, 4.f, "%.2f x");
				if (cfg.muted) ImGui::EndDisabled();
				ImGui::SameLine();
				if (ImGui::Button("+", ImVec2(20, 0))) cfg.gain = std::min(cfg.gain + 0.25f, 4.f);
				ImGui::SameLine();
				if (ImGui::Button("-", ImVec2(20, 0))) cfg.gain = std::max(cfg.gain - 0.25f, 0.f);
				ImGui::SameLine();
				if (ImGui::Button("R", ImVec2(20, 0))) { cfg.gain = 1.f; cfg.muted = false; }
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset (głośność = 1.0, odcisz)");

				fs::voiceReceivingManager.SetPlayerVoiceSettings(gameID, cfg);
			}

			ImGui::Separator();
			ImGui::PopID();
		}

		ImGui::EndChild();
	}

	ImGui::Separator();
	ImGui::Spacing();

	float innerW = ImGui::GetContentRegionAvail().x;
	float halfW  = (innerW - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

	// Bulk mute controls
	if (ImGui::Button("Wycisz wszystkich", ImVec2(halfW, 0))) {
		for (auto& [gameID, cfg] : m_voiceSettings) {
			if (all.count(gameID) && all.at(gameID).IsSelf()) continue;
			cfg.muted = true;
			fs::voiceReceivingManager.SetPlayerVoiceSettings(gameID, cfg);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Odcisz wszystkich", ImVec2(halfW, 0))) {
		for (auto& [gameID, cfg] : m_voiceSettings) {
			if (all.count(gameID) && all.at(gameID).IsSelf()) continue;
			cfg.muted = false;
			fs::voiceReceivingManager.SetPlayerVoiceSettings(gameID, cfg);
		}
	}

	ImGui::Spacing();

	// Microphone toggle
	bool isStopCapture = fs::voiceCaptureManager.IsStopCapture();
	const char* micLabel = isStopCapture ? "Włącz mikrofon" : "Wycisz mikrofon";
	if (ImGui::Button(micLabel, ImVec2(innerW, 30.f)))
		fs::voiceCaptureManager.StopCapture(!isStopCapture);

	ImGui::Spacing();

	// Invite button (only for host)
	if (fs::networking.Server() && !s_hostIP.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.10f, 0.35f, 0.60f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.20f, 0.55f, 0.85f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.05f, 0.25f, 0.50f, 1.f));
		if (ImGui::Button("Zaproś uczestników...", ImVec2(innerW, 0)))
			m_showInvitePopup = true;
		ImGui::PopStyleColor(3);
	}

	ImGui::End();

	UpdateInviteModal();
}

// --------------------------------------------------------------------------
// Chat - right panel
// --------------------------------------------------------------------------
void Lobby::UpdateChat()
{
	ImGuiViewport* vp    = ImGui::GetMainViewport();
	const float    chatX = vp->WorkPos.x + kLeftPanelWidth;
	const float    chatW = vp->WorkSize.x - kLeftPanelWidth;

	ImGui::SetNextWindowPos(ImVec2(chatX, vp->WorkPos.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(chatW, vp->WorkSize.y), ImGuiCond_Always);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.08f, 0.97f));
	ImGui::Begin("##ChatPanel", nullptr, flags);
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	// Header
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.30f, 1.f, 1.f));
	ImGui::SetWindowFontScale(1.05f);
	ImGui::TextUnformatted("CZAT");
	ImGui::SetWindowFontScale(1.f);
	ImGui::PopStyleColor();
	ImGui::Separator();

	// --- Reserve bottom area height ---
	const float kBarHeight    = ImGui::GetFrameHeight();
	const float kSpacing      = ImGui::GetStyle().ItemSpacing.y;
	const uint32_t numTransfers = (uint32_t)m_fileProgress.size();
	const float kProgressH    = numTransfers > 0
		? kSpacing + kBarHeight + kSpacing
		  + numTransfers * (kBarHeight + kSpacing)
		  + kSpacing
		: 0.f;
	const float kInputAreaH   = kBarHeight * 2.f + kSpacing * 5.f + 6.f + kProgressH;

	// --- Messages area ---
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.03f, 0.03f, 0.06f, 0.80f));
	ImGui::BeginChild("##ChatMessages", ImVec2(0, -kInputAreaH), false);
	ImGui::PopStyleColor();
	ImGui::Spacing();

	for (size_t i = 0; i < m_messages.size(); ++i) {
		const ChatMessage& msg = *m_messages[i];
		ImGui::PushID((int)i);

		// Author label
		ImGui::TextColored(ImVec4(0.65f, 0.40f, 1.f, 1.f), "%s", msg.authorLabel.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled(":");
		ImGui::SameLine();

		if (msg.type == ChatMessage::Type::Text) {
			ImGui::TextWrapped("%s", msg.text.c_str());
		}
		else if (msg.type == ChatMessage::Type::Image) {
			if (msg.imageTexture && msg.imageTexture->GetHandle()) {
				ImGui::NewLine();
				ImGui::Image((ImTextureID)msg.imageTexture->GetHandle(),
				             ImVec2(msg.imageW, msg.imageH));
				if (ImGui::IsItemHovered()) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(msg.imagePath.c_str());
					ImGui::TextDisabled("Kliknij, aby otworzyć");
					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked()) {
#ifdef _WIN32
					ShellExecuteA(nullptr, "open", msg.imagePath.c_str(),
					              nullptr, nullptr, SW_SHOWNORMAL);
#endif
				}
			} else {
				ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "[Błąd ładowania obrazka]");
			}
		}
		else if (msg.type == ChatMessage::Type::FileReceived) {
			ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.f),
			                   "[Plik: %s  %s]",
			                   msg.text.c_str(), FormatBytes(msg.fileSize).c_str());
			if (ImGui::IsItemHovered()) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(msg.savedPath.c_str());
				ImGui::TextDisabled("Kliknij, aby otworzyć");
				ImGui::EndTooltip();
			}
			if (ImGui::IsItemClicked()) {
#ifdef _WIN32
				ShellExecuteA(nullptr, "open", msg.savedPath.c_str(),
				              nullptr, nullptr, SW_SHOWNORMAL);
#endif
			}
		}
		else { // FileSent
			ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.35f, 1.f),
			                   "[Wysłano: %s]", msg.text.c_str());
			if (ImGui::IsItemHovered()) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(msg.savedPath.c_str());
				ImGui::TextDisabled("Kliknij, aby otworzyć");
				ImGui::EndTooltip();
			}
			if (ImGui::IsItemClicked()) {
#ifdef _WIN32
				ShellExecuteA(nullptr, "open", msg.savedPath.c_str(),
				              nullptr, nullptr, SW_SHOWNORMAL);
#endif
			}
		}

		ImGui::Spacing();
		ImGui::PopID();
	}

	if (m_scrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.f);
	m_scrollToBottom = false;

	ImGui::EndChild();

	// Handle pending file drop
	if (!m_pendingDropPath.empty()) {
		AttachFile(m_pendingDropPath);
		m_pendingDropPath.clear();
	}

	// --- Active file transfers ---
	if (!m_fileProgress.empty()) {
		ImGui::Separator();
		ImGui::TextDisabled("PRZESYŁANIE PLIKÓW:");
		for (const auto& [id, prog] : m_fileProgress) {
			const float frac = prog.totalChunks > 0
				? (float)prog.chunksReceived / (float)prog.totalChunks : 0.f;
			char overlay[256];
			snprintf(overlay, sizeof(overlay), "%s  %u/%u",
			         prog.filename.c_str(), prog.chunksReceived, prog.totalChunks);
			ImGui::PushID((int)id);
			ImGui::ProgressBar(frac, ImVec2(-1, 0), overlay);
			ImGui::PopID();
		}
	}

	ImGui::Separator();
	ImGui::Spacing();

	// --- Input row: message + Send ---
	{
		const float sendW  = 80.f;
		const float inputW = ImGui::GetContentRegionAvail().x
		                     - sendW - ImGui::GetStyle().ItemSpacing.x;

		ImGui::SetNextItemWidth(inputW);
		bool enter = ImGui::InputText("##msg", m_inputBuf, sizeof(m_inputBuf),
		                              ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f, 0.10f, 0.55f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.45f, 0.20f, 0.85f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.35f, 0.10f, 0.75f, 1.f));

		if ((ImGui::Button("Wyślij", ImVec2(sendW, 0)) || enter) && m_inputBuf[0] != '\0') {
			const auto& all = fs::connections.GetAll();
			auto [selfID, selfLabel] = FindSelf(all,
				[this](uint8_t id, bool s, bool h) { return GetClientLabel(id, s, h); });

			PostTextMessage(selfID, selfLabel, m_inputBuf);
			chatManager.Send(selfID, m_inputBuf);

			m_inputBuf[0] = '\0';
			ImGui::SetKeyboardFocusHere(-1);
		}
		ImGui::PopStyleColor(3);
	}

	// --- File row: preview + Choose file ---
	{
		const float btnW    = 130.f;
		const float previewW = ImGui::GetContentRegionAvail().x
		                       - btnW - ImGui::GetStyle().ItemSpacing.x;

		std::string previewStr = m_lastSelectedFile.empty()
			? "Przeciagnij plik lub kliknij Wybierz..."
			: std::filesystem::path(m_lastSelectedFile).filename().string();

		char previewBuf[512];
		strncpy(previewBuf, previewStr.c_str(), sizeof(previewBuf) - 1);
		previewBuf[sizeof(previewBuf) - 1] = '\0';

		ImGui::BeginDisabled();
		ImGui::SetNextItemWidth(previewW);
		ImGui::InputText("##preview", previewBuf, sizeof(previewBuf));
		ImGui::EndDisabled();
		ImGui::SameLine();

		if (ImGui::Button("Wybierz plik", ImVec2(btnW, 0))) {
			IGFD::FileDialogConfig config;
			config.path  = ".";
			config.flags = ImGuiFileDialogFlags_Modal;
			ImGuiFileDialog::Instance()->OpenDialog(
				"ChooseFileDlg",
				"Wybierz plik lub obraz",
				"Obrazy{.png,.jpg,.jpeg,.bmp,.gif,.tga},.*",
				config);
		}
	}

	// --- ImGuiFileDialog modal ---
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlg",
	    ImGuiWindowFlags_NoCollapse, ImVec2(500, 350), ImVec2(900, 600)))
	{
		if (ImGuiFileDialog::Instance()->IsOk()) {
			m_lastSelectedFile = ImGuiFileDialog::Instance()->GetFilePathName();
			AttachFile(m_lastSelectedFile);
		}
		ImGuiFileDialog::Instance()->Close();
	}

	ImGui::End();
}
