#include "states/LoginScreen.hpp"
#include "states/Lobby.hpp"

#include <imgui/imgui.h>
#include "networking/Networking.hpp"
#include "networking/Callback.hpp"

#if !USE_STEAM_GNS
#include "steam/steamnetworkingsockets.h"
#endif

#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <algorithm>

// Networking headers above already pull in windows.h + winsock2.h via Steam.
// We only need the extra Windows headers not included by Steam.
#ifdef _WIN32
#include <iphlpapi.h>
#include <shellapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

using namespace fs;

// --------------------------------------------------------------------------
// UserProfile
// --------------------------------------------------------------------------
static const char* kProfilePath = "user_profile.txt";

std::string UserProfile::GetDisplayName() const
{
	std::string s(firstName);
	if (lastName[0]) { s += ' '; s += lastName; }
	return s;
}

void UserProfile::Save() const
{
	std::ofstream f(kProfilePath);
	if (f) f << firstName << '\n' << lastName << '\n';
}

void UserProfile::Load()
{
	std::ifstream f(kProfilePath);
	if (f) {
		f.getline(firstName, sizeof(firstName));
		f.getline(lastName,  sizeof(lastName));
	}
}

// --------------------------------------------------------------------------
// IPv4 parsing
// --------------------------------------------------------------------------
static bool ParseIPv4(const char* ipStr, uint16_t port, SteamNetworkingIPAddr& out)
{
	unsigned int b1, b2, b3, b4;
	if (sscanf(ipStr, "%u.%u.%u.%u", &b1, &b2, &b3, &b4) != 4) return false;
	if (b1 > 255 || b2 > 255 || b3 > 255 || b4 > 255) return false;
	out.SetIPv4((b1 << 24) | (b2 << 16) | (b3 << 8) | b4, port);
	return true;
}

// --------------------------------------------------------------------------
// Local IP detection
// --------------------------------------------------------------------------
std::string LoginScreen::DetectLocalIP()
{
#ifdef _WIN32
	char buf[sizeof(IP_ADAPTER_INFO) * 16];
	ULONG bufLen = sizeof(buf);
	auto* info = reinterpret_cast<IP_ADAPTER_INFO*>(buf);

	if (GetAdaptersInfo(info, &bufLen) == NO_ERROR) {
		for (auto* p = info; p; p = p->Next) {
			std::string ip = p->IpAddressList.IpAddress.String;
			if (ip != "0.0.0.0" && ip != "127.0.0.1" && !ip.empty())
				return ip;
		}
	}
#endif
	return "127.0.0.1";
}

// --------------------------------------------------------------------------
// Protocol registration (vtlan://)
// --------------------------------------------------------------------------
void LoginScreen::RegisterVtlanProtocol()
{
#ifdef _WIN32
	char exePath[MAX_PATH] = {};
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);

	auto createKey = [](const char* path, HKEY& outKey) -> bool {
		return RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, nullptr,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &outKey, nullptr) == ERROR_SUCCESS;
	};
	auto setStr = [](HKEY key, const char* name, const char* val) {
		RegSetValueExA(key, name, 0, REG_SZ, (const BYTE*)val, (DWORD)strlen(val) + 1);
	};

	HKEY root;
	if (createKey("Software\\Classes\\vtlan", root)) {
		setStr(root, "",             "VT-LAN Audio Diagnostics");
		setStr(root, "URL Protocol", "");
		RegCloseKey(root);
	}
	HKEY cmdKey;
	if (createKey("Software\\Classes\\vtlan\\shell\\open\\command", cmdKey)) {
		std::string cmd = std::string("\"") + exePath + "\" \"%1\"";
		setStr(cmdKey, "", cmd.c_str());
		RegCloseKey(cmdKey);
	}
#endif
}

// --------------------------------------------------------------------------
// Clipboard helpers
// --------------------------------------------------------------------------
void LoginScreen::CopyToClipboard(const std::string& text) const
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

std::string LoginScreen::BuildInviteMessage() const
{
	std::string link = "vtlan://" + m_sessionIP + ":" + std::to_string(m_sessionPort);
	return
		"Zaproszenie do spotkania prowadzonego w srodowisku VT-LAN\n"
		"Kliknij ponizszy link, aby dolaczyc:\n"
		+ link + "\n\n"
		"Jesli link nie dziala, otworz aplikacje VT-LAN z nastepujacymi parametrami:\n"
		"  Adres IP: " + m_sessionIP + "\n"
		"  Port:     " + std::to_string(m_sessionPort);
}

void LoginScreen::OpenEmailInvite() const
{
#ifdef _WIN32
	// Simple URL encoding for subject and body
	auto encode = [](const std::string& s) {
		std::string out;
		for (unsigned char c : s) {
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '\n') {
				if (c == '\n') out += "%0A";
				else           out += (char)c;
			} else {
				char h[4];
				snprintf(h, sizeof(h), "%%%02X", c);
				out += h;
			}
		}
		return out;
	};

	std::string subject = "Zaproszenie do VT-LAN";
	std::string body    = BuildInviteMessage();
	std::string mailto  = "mailto:?subject=" + encode(subject) + "&body=" + encode(body);

	ShellExecuteA(nullptr, "open", mailto.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

// --------------------------------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------------------------------
LoginScreen::LoginScreen()
{
	RegisterVtlanProtocol();
	m_profile.Load();

	// Prefill from vtlan:// link
	if (!g_cmdPrefilledIP.empty() && g_cmdPrefilledPort > 0) {
		strncpy(m_joinIPBuf, g_cmdPrefilledIP.c_str(), sizeof(m_joinIPBuf) - 1);
		m_joinPort = g_cmdPrefilledPort;
		m_mode     = Mode::JoinRoom;
		if (!g_cmdPrefilledPassword.empty()) {
			strncpy(m_passwordBuf, g_cmdPrefilledPassword.c_str(), sizeof(m_passwordBuf) - 1);
			g_cmdPrefilledPassword.clear();
		}
		// If a profile name is already saved, skip the form and auto-connect.
		// Otherwise the form is pre-filled; the user fills in their name and clicks Join.
		if (m_profile.firstName[0] != '\0')
			m_autoConnect = true;
		g_cmdPrefilledIP.clear();
		g_cmdPrefilledPort = 0;
	}

	// Auto-detect local IP for host mode
	std::string localIP = DetectLocalIP();
	strncpy(m_hostIPBuf, localIP.c_str(), sizeof(m_hostIPBuf) - 1);
}

LoginScreen::~LoginScreen()
{
	CleanupCallbacks();
}

// --------------------------------------------------------------------------
void LoginScreen::Update()
{
	// Kicked out of Lobby (e.g. wrong password) — show reason and return to join form.
	if (!Lobby::s_disconnectedReason.empty()) {
		m_errorMsg = Lobby::s_disconnectedReason;
		Lobby::s_disconnectedReason.clear();
		m_mode = Mode::JoinRoom;
	}

	// Deferred Lobby push: syncedPlayersWhenJoined fired → we are fully in the session.
	// Done here (not inside the callback) to avoid pushing a state mid-callback.
	if (m_connectSuccess) {
		m_connectSuccess = false;
		CleanupCallbacks();
		StateMachine.PushTop(new Lobby);
		return;
	}

	// Auto-connect: launched via vtlan:// link and profile name is already saved.
	// Skip the form entirely and go straight to the loading screen.
	if (m_autoConnect) {
		m_autoConnect = false;
		m_profile.Save();
		Lobby::s_localDisplayName = m_profile.GetDisplayName();
		Lobby::s_joinPassword     = m_passwordBuf; // empty unless user typed a password
		BeginConnecting(m_joinIPBuf, static_cast<uint16_t>(m_joinPort));
		return;
	}

	switch (m_mode) {
	case Mode::SelectMode:  RenderSelectMode();  break;
	case Mode::CreateRoom:  RenderCreateRoom();  break;
	case Mode::JoinRoom:    RenderJoinRoom();    break;
	case Mode::Connecting:  RenderConnecting();  break;
	case Mode::SessionInfo: RenderSessionInfo(); break;
	}
}

void LoginScreen::Render() {}

// ==========================================================================
// Helpers for uniform window setup
// ==========================================================================
static void BeginCenteredWindow(const char* id, float w, float h, ImGuiWindowFlags extra = 0)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(
		{ (io.DisplaySize.x - w) * 0.5f, (io.DisplaySize.y - h) * 0.5f },
		ImGuiCond_Always);
	ImGui::SetNextWindowSize({ w, h }, ImGuiCond_Always);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings | extra;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.f, 20.f));
	ImGui::Begin(id, nullptr, flags);
	ImGui::PopStyleVar();
}

static void TitleLabel(const char* text, ImVec4 col = ImVec4(0.65f, 0.30f, 1.f, 1.f))
{
	ImGui::PushStyleColor(ImGuiCol_Text, col);
	ImGui::SetWindowFontScale(1.15f);
	ImGui::TextUnformatted(text);
	ImGui::SetWindowFontScale(1.f);
	ImGui::PopStyleColor();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
}

// Row helper: label on the left, input widget on the right
static void FieldLabel(const char* label, float labelW = 140.f)
{
	ImGui::Text("%s", label);
	ImGui::SameLine(labelW);
}

// ==========================================================================
// SelectMode
// ==========================================================================
void LoginScreen::RenderSelectMode()
{
	const float W = 380.f, H = 250.f;
	BeginCenteredWindow("##SelectMode", W, H);

	TitleLabel("VT-LAN");

	ImGui::TextWrapped("Wybierz sposób połączenia:");
	ImGui::Spacing();

	const float bW = (W - 48.f - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f, 0.10f, 0.55f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.45f, 0.20f, 0.85f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.35f, 0.10f, 0.75f, 1.f));

	if (ImGui::Button("Stwórz pokój", ImVec2(bW, 60.f)))
		m_mode = Mode::CreateRoom;

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.10f, 0.25f, 0.55f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.20f, 0.45f, 0.85f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.10f, 0.35f, 0.75f, 1.f));

	if (ImGui::Button("Dołącz do pokoju", ImVec2(bW, 60.f)))
		m_mode = Mode::JoinRoom;

	ImGui::PopStyleColor(6);

	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
	ImGui::TextDisabled("Wersja: VT-LAN Audio 1.0");

	ImGui::End();
}

// ==========================================================================
// CreateRoom
// ==========================================================================
void LoginScreen::RenderCreateRoom()
{
	const float W = 420.f, H = 390.f;
	BeginCenteredWindow("##CreateRoom", W, H);

	TitleLabel("Stwórz pokój");

	const float labelW = 140.f;
	const float fieldW = W - 48.f - labelW - ImGui::GetStyle().ItemSpacing.x;

	// --- Name ---
	FieldLabel("Imię:", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputText("##cr_fn", m_profile.firstName, sizeof(m_profile.firstName));

	FieldLabel("Nazwisko:", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputText("##cr_ln", m_profile.lastName, sizeof(m_profile.lastName));

	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

	// --- Network ---
	FieldLabel("Adres IP (lokalny):", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputText("##cr_ip", m_hostIPBuf, sizeof(m_hostIPBuf));

	FieldLabel("Port:", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputInt("##cr_port", &m_hostPort);
	m_hostPort = std::clamp(m_hostPort, 1, 65535);

	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

	// --- Password ---
	FieldLabel("Hasło (opcja):", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputText("##cr_pwd", m_passwordBuf, sizeof(m_passwordBuf),
	                 ImGuiInputTextFlags_Password);
	ImGui::SetCursorPosX(labelW + ImGui::GetStyle().ItemSpacing.x + 2.f);
	ImGui::TextDisabled("(zalecane)");

	ImGui::Spacing();

	// --- Error ---
	if (!m_errorMsg.empty())
		ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", m_errorMsg.c_str());

	ImGui::Spacing();

	// --- Buttons ---
	const float bW = (W - 48.f - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

	if (ImGui::Button("Wstecz", ImVec2(bW, 34.f))) {
		m_errorMsg.clear();
		m_mode = Mode::SelectMode;
	}
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button,       ImVec4(0.15f, 0.55f, 0.15f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.75f, 0.25f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f, 0.45f, 0.10f, 1.f));

	if (ImGui::Button("Utwórz pokój  >>", ImVec2(bW, 34.f))) {
		m_errorMsg.clear();
		if (m_profile.firstName[0] == '\0') {
			m_errorMsg = "Prosze podac swoje imie.";
		} else {
			SteamNetworkingIPAddr addr;
			if (!ParseIPv4(m_hostIPBuf, (uint16_t)m_hostPort, addr)) {
				m_errorMsg = "Nieprawidlowy adres IP.";
			} else {
				m_profile.Save();
				Lobby::s_roomPassword     = m_passwordBuf;
				Lobby::s_localDisplayName = m_profile.GetDisplayName();
				Lobby::s_hostIP           = m_hostIPBuf;
				Lobby::s_hostPort         = m_hostPort;
				fs::networking.StartListening(fs::NetworkingHostingMode::Local, addr);
				// Push Lobby immediately so it exists before any client can connect.
				// The invite modal auto-opens inside Lobby via s_showInviteOnStart.
				Lobby::s_showInviteOnStart = true;
				StateMachine.PushTop(new Lobby);
			}
		}
	}
	ImGui::PopStyleColor(3);

	ImGui::End();
}

// ==========================================================================
// JoinRoom
// ==========================================================================
void LoginScreen::RenderJoinRoom()
{
	const float W = 420.f, H = 340.f;
	BeginCenteredWindow("##JoinRoom", W, H);

	TitleLabel("Dołącz do pokoju");

	const float labelW = 140.f;
	const float fieldW = W - 48.f - labelW - ImGui::GetStyle().ItemSpacing.x;

	// --- Name ---
	FieldLabel("Imię:", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputText("##jr_fn", m_profile.firstName, sizeof(m_profile.firstName));

	FieldLabel("Nazwisko:", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputText("##jr_ln", m_profile.lastName, sizeof(m_profile.lastName));

	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

	// --- Network ---
	FieldLabel("Adres IP:", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputText("##jr_ip", m_joinIPBuf, sizeof(m_joinIPBuf));

	FieldLabel("Port:", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputInt("##jr_port", &m_joinPort);
	m_joinPort = std::clamp(m_joinPort, 1, 65535);

	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

	// --- Password ---
	FieldLabel("Hasło:", labelW);
	ImGui::SetNextItemWidth(fieldW);
	ImGui::InputText("##jr_pwd", m_passwordBuf, sizeof(m_passwordBuf),
	                 ImGuiInputTextFlags_Password);
	ImGui::SetCursorPosX(labelW + ImGui::GetStyle().ItemSpacing.x + 2.f);
	ImGui::TextDisabled("Wpisz jeżeli pokój jest chroniony");

	ImGui::Spacing();

	// --- Error ---
	if (!m_errorMsg.empty())
		ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", m_errorMsg.c_str());

	ImGui::Spacing();

	// --- Buttons ---
	const float bW = (W - 48.f - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

	if (ImGui::Button("Wstecz", ImVec2(bW, 34.f))) {
		m_errorMsg.clear();
		m_mode = Mode::SelectMode;
	}
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.10f, 0.25f, 0.55f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.20f, 0.45f, 0.85f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.10f, 0.35f, 0.75f, 1.f));

	if (ImGui::Button("Dołącz  >>", ImVec2(bW, 34.f))) {
		m_errorMsg.clear();
		if (m_profile.firstName[0] == '\0') {
			m_errorMsg = "Prosze podac swoje imie.";
		} else {
			m_profile.Save();
			Lobby::s_localDisplayName = m_profile.GetDisplayName();
			Lobby::s_joinPassword     = m_passwordBuf;
			BeginConnecting(m_joinIPBuf, static_cast<uint16_t>(m_joinPort));
		}
	}
	ImGui::PopStyleColor(3);

	ImGui::End();
}

// ==========================================================================
// BeginConnecting  –  initiate GNS connection and switch to loading mode
// ==========================================================================
void LoginScreen::BeginConnecting(const char* ip, uint16_t port)
{
	// Clean up any leftover callbacks from a previous attempt
	CleanupCallbacks();

	SteamNetworkingIPAddr addr;
	if (!ParseIPv4(ip, port, addr)) {
		m_errorMsg = "Nieprawidlowy adres IP.";
		m_mode = Mode::JoinRoom;
		return;
	}

	m_connectingDisplay = std::string(ip) + ":" + std::to_string(port);
	m_connectFailed     = false;
	m_connectSuccess    = false;
	m_connectErrorMsg.clear();

	// Create a scoped callback group — all Add() calls below are tracked and
	// will be removed when CleanupCallbacks() destroys this group.
	m_cbGroup = new fs::CallbackGroup();

	// Success: server has sent NewPlayerSync, we are fully in the session.
	callbacks.Get<fs::CallbackType::syncedPlayersWhenJoined>().Add(
		new fs::Callback<fs::CallbackType::syncedPlayersWhenJoined>([this]() {
			m_connectSuccess = true;
		})
	);

	// Immediate failure: ConnectByIPAddress returned an invalid handle.
	callbacks.Get<fs::CallbackType::clientConnectStatus>().Add(
		new fs::Callback<fs::CallbackType::clientConnectStatus>([this](fs::ClientStatus s) {
			if (s == fs::ClientStatus::InvalidConnection) {
				m_connectFailed   = true;
				m_connectErrorMsg = "Nie udalo sie polaczyc z\n" + m_connectingDisplay;
			}
		})
	);

	// Delayed failure: server unreachable / connection timed out / rejected.
	// ConnectionStateProblemDetectedLocally / ClosedByPeer both call
	// DisconnectFromServer() which fires clientDisconnectStatus(Disconnecting).
	callbacks.Get<fs::CallbackType::clientDisconnectStatus>().Add(
		new fs::Callback<fs::CallbackType::clientDisconnectStatus>([this](fs::ClientStatus s) {
			if (s == fs::ClientStatus::Disconnecting && m_mode == Mode::Connecting) {
				m_connectFailed   = true;
				m_connectErrorMsg = "Nie udalo sie polaczyc z\n" + m_connectingDisplay;
			}
		})
	);

	fs::networking.ConnectToServer(addr);
	m_mode = Mode::Connecting;
}

// ==========================================================================
// CleanupCallbacks  –  free callbacks registered for the current attempt
// ==========================================================================
void LoginScreen::CleanupCallbacks()
{
	if (m_cbGroup) {
		m_cbGroup->Free();  // removes + deletes all tracked callbacks immediately
		delete m_cbGroup;
		m_cbGroup = nullptr;
	}
}

// ==========================================================================
// RenderConnecting  –  loading screen + error popup
// ==========================================================================
void LoginScreen::RenderConnecting()
{
	const float W = 440.f, H = 210.f;
	BeginCenteredWindow("##Connecting", W, H);

	TitleLabel("Łączenie z serwerem...");

	// Animated spinner character
	static const char kSpin[] = "|/-\\";
	const int spinIdx = static_cast<int>(ImGui::GetTime() * 5.0) % 4;

	ImGui::SetWindowFontScale(1.05f);
	ImGui::TextColored(ImVec4(0.55f, 0.80f, 1.f, 1.f),
		" %c  %s", kSpin[spinIdx], m_connectingDisplay.c_str());
	ImGui::SetWindowFontScale(1.f);

	ImGui::Spacing();
	ImGui::TextDisabled("Proszę czekać na potwierdzenie połączenia...");
	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

	const float bW = W - 48.f;
	if (ImGui::Button("Anuluj", ImVec2(bW, 34.f))) {
		// Remove callbacks first so they don't fire during DisconnectFromServer().
		CleanupCallbacks();
		fs::networking.DisconnectFromServer();
		m_connectFailed = false;
		m_mode = Mode::JoinRoom;
	}

	ImGui::End();

	// Error popup — opened every frame while m_connectFailed is true so it
	// stays on screen until the user acknowledges it.
	if (m_connectFailed)
		ImGui::OpenPopup("Błąd połączenia##connect_err");

	ImVec2 popupSz(420.f, 190.f);
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(
		{ (io.DisplaySize.x - popupSz.x) * 0.5f,
		  (io.DisplaySize.y - popupSz.y) * 0.5f },
		ImGuiCond_Always);
	ImGui::SetNextWindowSize(popupSz, ImGuiCond_Always);

	if (ImGui::BeginPopupModal("Błąd połączenia##connect_err", nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.35f, 0.35f, 1.f));
		ImGui::SetWindowFontScale(1.05f);
		ImGui::TextUnformatted("Nie można połączyć");
		ImGui::SetWindowFontScale(1.f);
		ImGui::PopStyleColor();
		ImGui::Separator(); ImGui::Spacing();

		ImGui::TextWrapped("%s", m_connectErrorMsg.c_str());

		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		if (ImGui::Button("OK", ImVec2(popupSz.x - 48.f, 34.f))) {
			m_connectFailed = false;
			CleanupCallbacks();   // safe — we are NOT inside the callback here
			m_mode = Mode::JoinRoom;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

// ==========================================================================
// SessionInfo
// ==========================================================================
void LoginScreen::RenderSessionInfo()
{
	const float W = 460.f, H = 330.f;
	BeginCenteredWindow("##SessionInfo", W, H);

	TitleLabel("Pokój został utworzony!", ImVec4(0.3f, 0.9f, 0.4f, 1.f));

	// --- Info block ---
	const float lw = 130.f;

	ImGui::Text("Adres IP:");
	ImGui::SameLine(lw);
	ImGui::TextColored(ImVec4(1.f, 1.f, 0.6f, 1.f), "%s", m_sessionIP.c_str());

	ImGui::Text("Port:");
	ImGui::SameLine(lw);
	ImGui::TextColored(ImVec4(1.f, 1.f, 0.6f, 1.f), "%d", m_sessionPort);

	std::string link = "vtlan://" + m_sessionIP + ":" + std::to_string(m_sessionPort);
	ImGui::Text("Link:");
	ImGui::SameLine(lw);
	ImGui::TextColored(ImVec4(0.55f, 0.80f, 1.f, 1.f), "%s", link.c_str());

	if (!Lobby::s_roomPassword.empty()) {
		ImGui::Text("Hasło:");
		ImGui::SameLine(lw);
		ImGui::TextColored(ImVec4(1.f, 0.75f, 0.3f, 1.f), "ustawione");
	} else {
		ImGui::Text("Hasło:");
		ImGui::SameLine(lw);
		ImGui::TextDisabled("brak (pokój publiczny)");
	}

	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
	ImGui::TextUnformatted("Zaproś innych uczestników:");
	ImGui::Spacing();

	const float bW = (W - 48.f - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

	if (ImGui::Button("Kopiuj zaproszenie", ImVec2(bW, 34.f))) {
		CopyToClipboard(BuildInviteMessage());
		m_inviteCopied = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Wyślij e-mailem", ImVec2(bW, 34.f)))
		OpenEmailInvite();

	if (m_inviteCopied) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), " Skopiowano!");
	}

	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f, 0.10f, 0.55f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.45f, 0.20f, 0.85f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.35f, 0.10f, 0.75f, 1.f));

	if (ImGui::Button("Wejdź do pokoju  >>", ImVec2(W - 48.f, 36.f))) {
		Lobby::s_localDisplayName = m_profile.GetDisplayName();
		StateMachine.PushTop(new Lobby);
	}

	ImGui::PopStyleColor(3);
	ImGui::End();
}
