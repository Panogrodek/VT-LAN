#pragma once
#include "State Machine.hpp"
#include <string>

// Forward-declare so we can store a raw pointer without a heavy include
namespace fs { class CallbackGroup; }

// --------------------------------------------------------------------------
// User profile - persisted to user_profile.txt across sessions
// --------------------------------------------------------------------------
struct UserProfile {
	char firstName[64] = {};
	char lastName[64]  = {};

	std::string GetDisplayName() const;
	void Save() const;
	void Load();
};

// Filled by main() when the app is launched via a vtlan:// link
extern std::string g_cmdPrefilledIP;
extern int         g_cmdPrefilledPort;
extern std::string g_cmdPrefilledPassword;

// --------------------------------------------------------------------------
class LoginScreen : public State {
public:
	LoginScreen();
	~LoginScreen();   // must free m_cbGroup

private:
	enum class Mode {
		SelectMode,   // choose: create room or join room
		CreateRoom,   // host setup form
		JoinRoom,     // client connection form
		Connecting,   // loading screen while GNS connects and NewPlayerSync fires
		SessionInfo   // post-create: show IP/port + invite options
	};

	virtual void Update() override;
	virtual void Render() override;

	void RenderSelectMode();
	void RenderCreateRoom();
	void RenderJoinRoom();
	void RenderConnecting();   // loading screen + error popup
	void RenderSessionInfo();

	// Initiate a client connection attempt and switch to Connecting mode.
	// Callers must set Lobby::s_localDisplayName / s_joinPassword beforehand.
	void BeginConnecting(const char* ip, uint16_t port);

	// Remove and delete all callbacks registered for the current connection attempt.
	void CleanupCallbacks();

	// --- data ---
	UserProfile m_profile;
	char        m_passwordBuf[64] = {};

	// Create Room
	char m_hostIPBuf[64] = {};
	int  m_hostPort      = 27020;

	// Join Room
	char m_joinIPBuf[64] = {};
	int  m_joinPort      = 27020;

	// Populated after creating a room
	std::string m_sessionIP;
	int         m_sessionPort = 0;

	Mode        m_mode     = Mode::SelectMode;
	std::string m_errorMsg;
	bool        m_inviteCopied = false; // brief feedback after copy

	// --- Connecting state ---
	std::string        m_connectingDisplay; // "IP:PORT" shown in loading UI
	std::string        m_connectErrorMsg;   // message shown in error popup
	bool               m_connectFailed  = false; // set by callback on failure
	bool               m_connectSuccess = false; // set by callback on success
	// When launched via vtlan:// link with a saved profile, skip the form
	// and auto-connect on the first Update() tick.
	bool               m_autoConnect    = false;
	fs::CallbackGroup* m_cbGroup        = nullptr;

	// --- helpers ---
	std::string BuildInviteMessage() const;
	void        CopyToClipboard(const std::string& text) const;
	void        OpenEmailInvite() const;

	static void        RegisterVtlanProtocol();
	static std::string DetectLocalIP();
};
