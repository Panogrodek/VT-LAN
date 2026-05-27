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

#include "states/State Machine.hpp"
#include "states/LoginScreen.hpp"
#include "states/Lobby.hpp"

static void ClientDisconnected(fs::ClientStatus status) {
	if (status == fs::ClientStatus::Disconnecting) {
		// Only clear and restart when Lobby is on top.
		// While LoginScreen is on top (user is in the connecting phase),
		// LoginScreen handles the failure itself — we must not destroy it.
		if (dynamic_cast<Lobby*>(StateMachine.GetTop()) != nullptr) {
			StateMachine.Clear();
			StateMachine.PushTop(new LoginScreen);
		}
	}
}

static void SDL_DROP(SDL_Event ev) {
	if (ev.type != SDL_EVENT_DROP_FILE)
		return;

	//TODO: fix this
	static_cast<Lobby*>(StateMachine.GetTop())->OnDropFile(ev.drop.data);
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

	StateMachine.PushTop(new LoginScreen);

	fs::callbacks.Get<fs::CallbackType::clientDisconnectStatus>().Add(new fs::Callback<fs::CallbackType::clientDisconnectStatus>(ClientDisconnected));
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

	StateMachine.Update();
	//UpdateVoiceConnections();


	//UpdateReceivedPlot();


	MessageManager.Update();
}

void Application::Render() {
	fs::Renderer.BuildSwapchainCommandBuffer();

	/*
	Render Start
	*/

	StateMachine.Render();

	/*
	Render End
	*/

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplSDLGPU3_PrepareDrawData(drawData, fs::Renderer.GetSwapchainCommandBuffer());

	fs::Renderer.DrawDataToBuffers();

	if (fs::Renderer.GetSwapchainTexture())
		ImGui_ImplSDLGPU3_RenderDrawData(drawData, fs::Renderer.GetSwapchainCommandBuffer(), fs::Renderer.GetFlushRenderPass());

	fs::Renderer.Flush();
}

void Application::UpdateMainMenu() {
	ImGui::BeginMainMenuBar();

	// App title
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.30f, 1.f, 1.f));
	ImGui::TextUnformatted("VT-LAN");
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::TextDisabled("|");
	ImGui::SameLine();

	// Connection status
	if (fs::networking.Server())
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.f), "HOST");
	else if (fs::networking.Client())
		ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.f), "Połączony");
	else
		ImGui::TextDisabled("Niepodłączony");

	// User name on the right side
	if (!Lobby::s_localDisplayName.empty()) {
		float nameW = ImGui::CalcTextSize(Lobby::s_localDisplayName.c_str()).x + 16.f;
		ImGui::SameLine(ImGui::GetWindowWidth() - nameW);
		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.f),
		                   "%s", Lobby::s_localDisplayName.c_str());
	}

	m_mainMenuBarHeight = ImGui::GetWindowSize().y;

	ImGui::EndMainMenuBar();
}

void Application::UpdateDefaultDockingSpace() {
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
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
	style.FontScaleDpi = 1.0f;
	io.ConfigDpiScaleFonts = true;
	io.ConfigDpiScaleViewports = true;

	ImGui_ImplSDL3_InitForSDLGPU(fs::Window.GetHandle());

	ImGui_ImplSDLGPU3_InitInfo init_info = {};
	init_info.Device = fs::RenderBackend.GetContext();
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(fs::RenderBackend.GetContext(), fs::Window.GetHandle());
	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
	init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
	ImGui_ImplSDLGPU3_Init(&init_info);

	SetEventCallback([](SDL_Event event) { 
		ImGui_ImplSDL3_ProcessEvent(&event);
		if (event.type == SDL_EVENT_DROP_FILE) {
			static_cast<Lobby*>(StateMachine.GetTop())->OnDropFile(event.drop.data);
		}
	});
}

void Application::InitializeImGuiStyle() {
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.FontDefault = io.Fonts->AddFontFromFileTTF("res/roboto-regular.ttf", 18.0f);

	ImGuiStyle& style = ImGui::GetStyle();

	// --- Shape ---
	style.WindowRounding    = 8.0f;
	style.FrameRounding     = 5.0f;
	style.ChildRounding     = 5.0f;
	style.PopupRounding     = 6.0f;
	style.ScrollbarRounding = 4.0f;
	style.GrabRounding      = 4.0f;
	style.TabRounding       = 5.0f;

	// --- Spacing ---
	style.WindowPadding     = ImVec2(12.f, 10.f);
	style.FramePadding      = ImVec2(8.f,  5.f);
	style.ItemSpacing       = ImVec2(8.f,  6.f);
	style.ItemInnerSpacing  = ImVec2(6.f,  4.f);
	style.ScrollbarSize     = 12.f;
	style.GrabMinSize       = 10.f;
	style.WindowBorderSize  = 1.f;
	style.FrameBorderSize   = 0.f;

	// --- Accent color: rich purple (#7B35C8) ---
	const float Ar = 0.48f, Ag = 0.20f, Ab = 0.78f; // accent

	ImVec4* c = style.Colors;

	c[ImGuiCol_Text]                  = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
	c[ImGuiCol_TextDisabled]          = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);

	c[ImGuiCol_WindowBg]              = ImVec4(0.07f, 0.06f, 0.10f, 0.97f);
	c[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	c[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.09f, 0.14f, 0.97f);

	c[ImGuiCol_Border]                = ImVec4(Ar * 0.6f, Ag * 0.6f, Ab * 0.6f, 0.55f);
	c[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	c[ImGuiCol_FrameBg]               = ImVec4(0.15f, 0.13f, 0.22f, 0.70f);
	c[ImGuiCol_FrameBgHovered]        = ImVec4(Ar, Ag, Ab, 0.35f);
	c[ImGuiCol_FrameBgActive]         = ImVec4(Ar, Ag, Ab, 0.60f);

	c[ImGuiCol_TitleBg]               = ImVec4(0.06f, 0.05f, 0.10f, 1.00f);
	c[ImGuiCol_TitleBgActive]         = ImVec4(Ar * 0.65f, Ag * 0.65f, Ab * 0.65f, 1.00f);
	c[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);

	c[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.09f, 0.14f, 1.00f);

	c[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.05f, 0.53f);
	c[ImGuiCol_ScrollbarGrab]         = ImVec4(Ar * 0.55f, Ag * 0.55f, Ab * 0.55f, 1.00f);
	c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(Ar * 0.80f, Ag * 0.80f, Ab * 0.80f, 1.00f);
	c[ImGuiCol_ScrollbarGrabActive]   = ImVec4(Ar, Ag, Ab, 1.00f);

	c[ImGuiCol_CheckMark]             = ImVec4(Ar + 0.15f, Ag + 0.10f, Ab + 0.10f, 1.00f);
	c[ImGuiCol_SliderGrab]            = ImVec4(Ar, Ag, Ab, 0.90f);
	c[ImGuiCol_SliderGrabActive]      = ImVec4(Ar + 0.10f, Ag + 0.05f, Ab + 0.10f, 1.00f);

	c[ImGuiCol_Button]                = ImVec4(Ar, Ag, Ab, 0.38f);
	c[ImGuiCol_ButtonHovered]         = ImVec4(Ar + 0.10f, Ag + 0.05f, Ab + 0.10f, 0.90f);
	c[ImGuiCol_ButtonActive]          = ImVec4(Ar - 0.05f, Ag, Ab - 0.05f, 1.00f);

	c[ImGuiCol_Header]                = ImVec4(Ar, Ag, Ab, 0.30f);
	c[ImGuiCol_HeaderHovered]         = ImVec4(Ar, Ag, Ab, 0.75f);
	c[ImGuiCol_HeaderActive]          = ImVec4(Ar, Ag, Ab, 1.00f);

	c[ImGuiCol_Separator]             = ImVec4(Ar * 0.4f, Ag * 0.4f, Ab * 0.5f, 0.60f);
	c[ImGuiCol_SeparatorHovered]      = ImVec4(Ar, Ag, Ab, 0.78f);
	c[ImGuiCol_SeparatorActive]       = ImVec4(Ar, Ag, Ab, 1.00f);

	c[ImGuiCol_ResizeGrip]            = ImVec4(Ar, Ag, Ab, 0.20f);
	c[ImGuiCol_ResizeGripHovered]     = ImVec4(Ar, Ag, Ab, 0.67f);
	c[ImGuiCol_ResizeGripActive]      = ImVec4(Ar, Ag, Ab, 0.95f);

	c[ImGuiCol_Tab]                   = ImVec4(Ar * 0.55f, Ag * 0.40f, Ab * 0.65f, 0.86f);
	c[ImGuiCol_TabHovered]            = ImVec4(Ar, Ag, Ab, 0.80f);
	c[ImGuiCol_TabSelected]           = ImVec4(Ar + 0.05f, Ag + 0.02f, Ab + 0.05f, 1.00f);
	c[ImGuiCol_TabSelectedOverline]   = ImVec4(Ar + 0.15f, Ag + 0.10f, Ab + 0.15f, 1.00f);
	c[ImGuiCol_TabDimmed]             = ImVec4(0.09f, 0.06f, 0.13f, 0.97f);
	c[ImGuiCol_TabDimmedSelected]     = ImVec4(Ar * 0.45f, Ag * 0.35f, Ab * 0.55f, 1.00f);
	c[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);

	c[ImGuiCol_DockingPreview]        = ImVec4(Ar, Ag, Ab, 0.70f);
	c[ImGuiCol_DockingEmptyBg]        = ImVec4(0.12f, 0.10f, 0.16f, 1.00f);

	c[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	c[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	c[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.45f, 0.60f, 1.00f);
	c[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);

	c[ImGuiCol_TableHeaderBg]         = ImVec4(0.16f, 0.14f, 0.22f, 1.00f);
	c[ImGuiCol_TableBorderStrong]     = ImVec4(0.28f, 0.26f, 0.35f, 1.00f);
	c[ImGuiCol_TableBorderLight]      = ImVec4(0.20f, 0.18f, 0.26f, 1.00f);
	c[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	c[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.05f);

	c[ImGuiCol_TextLink]              = ImVec4(Ar + 0.15f, Ag + 0.20f, Ab + 0.15f, 1.00f);
	c[ImGuiCol_TextSelectedBg]        = ImVec4(Ar, Ag, Ab, 0.40f);
	c[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	c[ImGuiCol_NavCursor]             = ImVec4(Ar + 0.15f, Ag + 0.10f, Ab + 0.15f, 1.00f);
	c[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	c[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	c[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.05f, 0.03f, 0.10f, 0.60f);
}