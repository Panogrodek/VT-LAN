#include "Application.hpp"
#include "tests/InstanceLauncher.hpp"

#include "networking/RPC.hpp"

#include <string>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Globals filled from vtlan:// command-line argument (consumed by LoginScreen on startup)
std::string g_cmdPrefilledIP;
int         g_cmdPrefilledPort = 0;
std::string g_cmdPrefilledPassword;

static std::string UrlDecode(const std::string& s)
{
	std::string out;
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == '%' && i + 2 < s.size()) {
			unsigned int val = 0;
			if (sscanf(s.c_str() + i + 1, "%2x", &val) == 1) {
				out += (char)val;
				i += 2;
			} else {
				out += s[i];
			}
		} else if (s[i] == '+') {
			out += ' ';
		} else {
			out += s[i];
		}
	}
	return out;
}

static void ParseVtlanArg(const char* arg)
{
	if (!arg) return;
	const std::string s(arg);
	const std::string prefix = "vtlan://";
	if (s.rfind(prefix, 0) != 0) return;

	std::string rest = s.substr(prefix.size());
	// strip trailing slashes
	while (!rest.empty() && (rest.back() == '/' || rest.back() == '\\'))
		rest.pop_back();

	// split off query string (?pwd=...)
	std::string query;
	auto qmark = rest.find('?');
	if (qmark != std::string::npos) {
		query = rest.substr(qmark + 1);
		rest  = rest.substr(0, qmark);
	}

	auto colon = rest.find(':');
	if (colon != std::string::npos) {
		g_cmdPrefilledIP = rest.substr(0, colon);
		try { g_cmdPrefilledPort = std::stoi(rest.substr(colon + 1)); }
		catch (...) { g_cmdPrefilledPort = 27020; }
	}

	// extract password from query string
	if (!query.empty()) {
		const std::string pwdKey = "pwd=";
		auto pos = query.find(pwdKey);
		if (pos != std::string::npos)
			g_cmdPrefilledPassword = UrlDecode(query.substr(pos + pwdKey.size()));
	}
}

constexpr bool launchMultipleInstances = 0;
constexpr int  childInstances = 0;

int main(int argc, char* argv[])
{
#ifdef _WIN32
	// When Windows launches the app via a vtlan:// protocol handler it sets the
	// working directory to C:\Windows\System32 instead of the app folder.
	// Fix this immediately so that relative asset paths ("res/shaders/...") work.
	{
		char exePath[MAX_PATH] = {};
		GetModuleFileNameA(nullptr, exePath, MAX_PATH);
		char* lastSep = strrchr(exePath, '\\');
		if (!lastSep) lastSep = strrchr(exePath, '/');
		if (lastSep) {
			*lastSep = '\0';
			SetCurrentDirectoryA(exePath);
		}
	}
#endif

	fs::getRPCMap().Lock();

	// Parse vtlan:// protocol link before anything else
	for (int i = 1; i < argc; ++i) {
		if (argv[i] && strncmp(argv[i], "vtlan://", 8) == 0) {
			ParseVtlanArg(argv[i]);
			break;
		}
	}



	{
		// If launched via vtlan:// deep link, disable child spawning to avoid
		// multiple instances all trying to connect to the same host.
		const bool multiInst = g_cmdPrefilledIP.empty() && launchMultipleInstances;
		fs::InstanceConfig cfg{ multiInst, childInstances };
		const fs::LaunchMode mode = fs::instanceManager.init(argc, argv, cfg);

		if (mode == fs::LaunchMode::Parent)
			fs::instanceManager.spawnChildrenAsync();
		else if (mode == fs::LaunchMode::Child)
			fs::instanceManager.startParentWatcherIfNeeded();
	}



	fs::WindowData data;
	data.Size  = glm::uvec2(1600 / 2, 900 / 2);
	data.Title = "VT-LAN";

	fs::Runtime::Initialize(data);

	Application app;
	app.Run();

	return 0;
}
