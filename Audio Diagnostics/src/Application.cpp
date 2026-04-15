#include "Application.hpp"

#include "utilities/Macros.hpp"

#include "system/Window.hpp"
#include "core/Render Backend.hpp"

#include "render/Renderer.hpp"

#include "managers/Camera Manager.hpp"
#include "managers/Resource Manager.hpp"

#include "window prompts/User Message.hpp"

#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlgpu3.h"
#include "imgui/implot.h"


#include "sdl3/SDL_events.h"
#include "sdl3/SDL_gpu.h" // this is kinda fucked that we need that here

#include "audio/Audio.hpp"

#include "networking/Networking.hpp"
#if !USE_STEAM_GNS
#include "steam/steamnetworkingsockets.h"
#endif
#include <tests/VoiceChatTest.hpp>

bool ParseIPv4(const char* ipStr, uint16 port, SteamNetworkingIPAddr& outAddr)
{
	unsigned int b1, b2, b3, b4;

	// Strict parsing: must match exactly 4 numbers
	if (sscanf(ipStr, "%u.%u.%u.%u", &b1, &b2, &b3, &b4) != 4)
		return false;

	// Range check
	if (b1 > 255 || b2 > 255 || b3 > 255 || b4 > 255)
		return false;

	uint32 ip =
		(b1 << 24) |
		(b2 << 16) |
		(b3 << 8) |
		(b4);

	outAddr.SetIPv4(ip, port);
	return true;
}

Application::Application() {
	SetExternalRender(true); // this ensures that imgui works because we need to render it externally

	LoadAssets();

	InitializeImGui();
	InitializeImGuiStyle();

	fs::AudioSourceManager.Load("sample", "res/audio/sample.wav", false);

	SDL_AudioSpec src{};
	src.freq = 48000; // 48000
	src.format = SDL_AUDIO_F32;              // signed 16-bit
	src.channels = 1;

	fs::audioDeviceManager.OpenDefaultDevice(fs::AudioDeviceKind::Playback);
	fs::audioDeviceManager.OpenDefaultDevice(fs::AudioDeviceKind::Recording, &src);
	
	fs::soundManager.Add(fs::Sound{ &fs::AudioSourceManager.Get("sample") },"sample");
	fs::soundManager.Get("sample")->SetVolume(0.2f);
}

Application::~Application() {
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplSDLGPU3_Shutdown();
	ImGui::DestroyContext();
}

void Application::Update() {
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = dt;

	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	UpdateDefaultDockingSpace();
	UpdateMainMenu();

	UpdateHostJoin();
	UpdateAudio();


	UpdateReceivedPlot();


	MessageManager.Update();
}

void Application::Render() {
	fs::Renderer.BuildSwapchainCommandBuffer();

	/*
	Render Start
	*/
	
	//...

	/*
	Render End
	*/

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplSDLGPU3_PrepareDrawData(drawData, fs::Renderer.GetSwapchainCommandBuffer());

	fs::Renderer.DrawDataToBuffers();

	if(fs::Renderer.GetSwapchainTexture())
		ImGui_ImplSDLGPU3_RenderDrawData(drawData, fs::Renderer.GetSwapchainCommandBuffer(), fs::Renderer.GetFlushRenderPass());

	fs::Renderer.Flush();
}

void Application::UpdateMainMenu() {
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("View")) {
		if (ImGui::MenuItem("Reset view")) {
			
		}

		ImGui::EndMenu();
	}

	m_mainMenuBarHeight = ImGui::GetWindowSize().y;

	ImGui::EndMainMenuBar();
}

void Application::UpdateDefaultDockingSpace() {
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoDocking			   |
		ImGuiWindowFlags_NoTitleBar			   |
		ImGuiWindowFlags_NoCollapse			   |
		ImGuiWindowFlags_NoResize			   |
		ImGuiWindowFlags_NoMove				   |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus			   |
		ImGuiWindowFlags_NoBackground;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + m_mainMenuBarHeight));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - m_mainMenuBarHeight));
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("DockSpaceHost", nullptr, windowFlags);
	ImGui::PopStyleVar(2);

	ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
	ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

	ImGui::End();
}

void Application::UpdateHostJoin()
{
	if (ImGui::Begin("Network"))
	{
		ImGui::Text("Choose an action:");
		ImGui::Separator();
		ImGui::Spacing();

		static char ipBuffer[64] = "127.0.0.1";
		static int port = 27020;

		ImGui::Text("Target:");
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputText("IP", ipBuffer, IM_ARRAYSIZE(ipBuffer));

		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputInt("Port", &port);

		if (port < 1) port = 1;
		if (port > 65535) port = 65535;

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		SteamNetworkingIPAddr previewAddr;
		bool ipOk = ParseIPv4(ipBuffer, static_cast<uint16>(port), previewAddr);

		if (!ipOk)
			ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "Invalid IP address.");

		ImGui::Spacing();

		if (ImGui::Button("Host", ImVec2(120, 40)))
		{
			SteamNetworkingIPAddr addr;
			if (ParseIPv4(ipBuffer, static_cast<uint16>(port), addr))
			{
				fs::networking.StartListening(fs::NetworkingHostingMode::Local, addr);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Connect", ImVec2(120, 40)))
		{
			SteamNetworkingIPAddr addr;
			if (ParseIPv4(ipBuffer, static_cast<uint16>(port), addr))
			{
				fs::networking.ConnectToServer(addr);
			}
		}

		ImGui::Spacing();
	}
	ImGui::End();
}

void Application::UpdateAudio()
{
	if (fs::connections.GetCount() < 2)
		return;

	// Persist across frames, but still "scoped" to this function.
	static float s_gain = 2.0f;
	static bool  s_muted = false;

	// --- UI ---
	ImGui::Begin("Voice Settings");
	ImGui::SliderFloat("Volume", &s_gain, 0.0f, 4.0f, "%.2f");
	ImGui::Checkbox("Mute other client", &s_muted);
	ImGui::End();

	// --- Apply ---
	fs::PlayerVoiceSettings playerSettings;
	playerSettings.gain = s_gain;
	playerSettings.muted = s_muted;

	const int targetPlayerId = fs::networking.Server() ? 2 : 1;
	fs::voiceReceivingManager.SetPlayerVoiceSettings(targetPlayerId, playerSettings);
}

void Application::UpdateReceivedPlot()
{
	ReceivedStatePlot();
	ReceivedBytesPlot();
}

void Application::ReceivedStatePlot()
{
	ImGui::Begin("Received State Plot");
	// 1) Snapshot into UI-local storage (thread-safe: Snapshot locks internally)
	std::vector<uint32_t> ticks;
	std::vector<uint32_t> state;
	fs::voiceChatTest.SnapshotReceivedStateData(ticks, state);

	// 2) Convert to ImPlot-friendly arrays (ImPlot wants numeric arrays; doubles are nice)
	std::vector<double> xSeconds;
	std::vector<double> y;

	const size_t n = ticks.size();
	xSeconds.reserve(n);
	y.reserve(n);

	constexpr double TICK_PERIOD_S = 0.001; // 1 tick = 0.001 [s]

	for (size_t i = 0; i < n; ++i) {
		xSeconds.push_back(static_cast<double>(ticks[i]) * TICK_PERIOD_S);
		y.push_back(static_cast<double>(state[i])); // 0/1/2
	}

	// 3) Plot
	if (ImPlot::BeginPlot("Voice Received State", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "ticks");
		ImPlot::SetupAxis(ImAxis_Y1, "state");
		ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, 2.5, ImGuiCond_Always);

		if (n > 0) {

			ImPlot::SetupAxisLimits(ImAxis_X1, xSeconds.front(), xSeconds.back(), ImGuiCond_Always);

			ImPlot::PlotStairs("state: 1 - normal, 2 - PLC", xSeconds.data(), y.data(), static_cast<int>(n));
		}

		ImPlot::EndPlot();
	}
	ImGui::End();
}

void Application::ReceivedBytesPlot()
{
	ImGui::Begin("Received Bytes Plot");
	// 1) Snapshot into UI-local storage (thread-safe: Snapshot locks internally)
	std::vector<uint32_t> ticks;
	std::vector<uint32_t> bytes;
	fs::voiceChatTest.SnapshotReceivedBytesData(ticks, bytes);

	// 2) Convert to ImPlot-friendly arrays (ImPlot wants numeric arrays; doubles are nice)
	std::vector<double> xSeconds;
	std::vector<double> y;

	const size_t n = ticks.size();
	xSeconds.reserve(n);
	y.reserve(n);

	constexpr double TICK_PERIOD_S = 0.001; // 1 tick = 0.001 [s]

	for (size_t i = 0; i < n; ++i) {
		xSeconds.push_back(static_cast<double>(ticks[i]) * TICK_PERIOD_S);
		y.push_back(static_cast<double>(bytes[i]));
	}

	// 3) Plot
	if (ImPlot::BeginPlot("Voice Received Bytes", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "ticks");
		ImPlot::SetupAxis(ImAxis_Y1, "bytes");


		if (n > 0) {

			ImPlot::SetupAxisLimits(ImAxis_X1, xSeconds.front(), xSeconds.back(), ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 500, ImGuiCond_Always);

			ImPlot::PlotStairs("bytes", xSeconds.data(), y.data(), static_cast<int>(n));
		}

		ImPlot::EndPlot();
	}
	ImGui::End();
}

void Application::LoadAssets() {

}

void Application::InitializeImGui() {
	DB_ONLY(IMGUI_CHECKVERSION());

	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(1.0f);       
	style.FontScaleDpi		   = 1.0f;
	io.ConfigDpiScaleFonts	   = true;     
	io.ConfigDpiScaleViewports = true;

	ImGui_ImplSDL3_InitForSDLGPU(fs::Window.GetHandle());

	ImGui_ImplSDLGPU3_InitInfo init_info = {};
	init_info.Device				= fs::RenderBackend.GetContext();
	init_info.ColorTargetFormat		= SDL_GetGPUSwapchainTextureFormat(fs::RenderBackend.GetContext(), fs::Window.GetHandle());
	init_info.MSAASamples			= SDL_GPU_SAMPLECOUNT_1;                    
	init_info.SwapchainComposition  = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
	init_info.PresentMode			= SDL_GPU_PRESENTMODE_VSYNC;
	ImGui_ImplSDLGPU3_Init(&init_info);

	SetEventCallback([](SDL_Event event){ ImGui_ImplSDL3_ProcessEvent(&event); });
}

void Application::InitializeImGuiStyle() {
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.FontDefault = io.Fonts->AddFontFromFileTTF("res/roboto-regular.ttf", 18.0f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding	= 5.3f;
    style.FrameRounding		= 2.3f;
    style.ScrollbarRounding = 0;

    ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text]						= ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]				= ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]					= ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_ChildBg]					= ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]					= ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border]						= ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow]				= ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg]					= ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]				= ImVec4(0.60f, 0.26f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive]				= ImVec4(0.60f, 0.26f, 0.98f, 0.67f);
	colors[ImGuiCol_TitleBg]					= ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive]				= ImVec4(0.31f, 0.16f, 0.48f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]			= ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg]					= ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]				= ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]				= ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]		= ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]		= ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark]					= ImVec4(0.60f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab]					= ImVec4(0.54f, 0.24f, 0.88f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]			= ImVec4(0.60f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_Button]						= ImVec4(0.60f, 0.26f, 0.98f, 0.40f);
	colors[ImGuiCol_ButtonHovered]				= ImVec4(0.60f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_ButtonActive]				= ImVec4(0.49f, 0.06f, 0.98f, 1.00f);
	colors[ImGuiCol_Header]						= ImVec4(0.60f, 0.26f, 0.98f, 0.31f);
	colors[ImGuiCol_HeaderHovered]				= ImVec4(0.60f, 0.26f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive]				= ImVec4(0.60f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator]					= ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_SeparatorHovered]			= ImVec4(0.41f, 0.10f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive]			= ImVec4(0.41f, 0.10f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip]					= ImVec4(0.60f, 0.26f, 0.98f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered]			= ImVec4(0.60f, 0.26f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]			= ImVec4(0.60f, 0.26f, 0.98f, 0.95f);
	colors[ImGuiCol_TabHovered]					= ImVec4(0.60f, 0.26f, 0.98f, 0.80f);
	colors[ImGuiCol_Tab]						= ImVec4(0.37f, 0.18f, 0.58f, 0.86f);
	colors[ImGuiCol_TabSelected]				= ImVec4(0.42f, 0.20f, 0.68f, 1.00f);
	colors[ImGuiCol_TabSelectedOverline]		= ImVec4(0.60f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_TabDimmed]					= ImVec4(0.11f, 0.07f, 0.15f, 0.97f);
	colors[ImGuiCol_TabDimmedSelected]			= ImVec4(0.27f, 0.14f, 0.42f, 1.00f);
	colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
	colors[ImGuiCol_DockingPreview]				= ImVec4(0.60f, 0.26f, 0.98f, 0.70f);
	colors[ImGuiCol_DockingEmptyBg]				= ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines]					= ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]			= ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]				= ImVec4(0.90f, 0.45f, 0.60f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]		= ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg]				= ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong]			= ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight]			= ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg]					= ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt]				= ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextLink]					= ImVec4(0.60f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]				= ImVec4(0.53f, 0.53f, 0.53f, 0.45f);
	colors[ImGuiCol_DragDropTarget]				= ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavCursor]					= ImVec4(0.60f, 0.26f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]		= ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]			= ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]			= ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
