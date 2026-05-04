#pragma once

#include "core/Runtime.hpp"

#include "graphics/Framebuffer.hpp"
#include "graphics/Quad.hpp"

#include "render/Render Pipeline.hpp"

class Application : public fs::Runtime {
public:
	Application();
	~Application();

private:
	float m_mainMenuBarHeight = 0.0f;
	glm::vec3 m_audioPos{};

	virtual void Update() override;
	virtual void Render() override;
	virtual void PhysicsUpdate() override {};

	void UpdateMainMenu();

	void UpdateDefaultDockingSpace();
	void UpdateHostJoin();
	void UpdateVoiceConnections();

	void UpdateReceivedPlot();
	void ReceivedStatePlot();
	void ReceivedBytesPlot();

	void LoadAssets();

	void InitializeImGui();
	void InitializeImGuiStyle();
};

