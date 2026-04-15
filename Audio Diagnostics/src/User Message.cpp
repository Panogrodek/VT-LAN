#include "window prompts/User Message.hpp"

#include "utilities/Macros.hpp"

#include "window prompts/Window Prompts.hpp"

#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlgpu3.h"

using namespace priv;

void MessageManager::Update() {
	for (int i = 0; i < m_Messages.size(); i++) {
		auto& [message, callback] = m_Messages[i];

		MessageButton button = message();
		if (button != MessageButton::Invalid) {
			if (callback)
				callback(button);

			m_Messages.erase(m_Messages.begin() + i);
			i--;
		}
	}
}

void MessageManager::Push(const std::string& title, const std::string& message, MessageButton buttons, std::function<void(MessageButton)> callback) {
	DB_ASSERT(!HasFlag(buttons, MessageButton::Invalid)); // invalid is not correct button

	uint32_t titleCount = ++m_TitleCounter[title];

	std::string fullTitle = title + "##" + std::to_string(titleCount);

	auto MessageUpdate = [fullTitle, message, buttons]() -> MessageButton {
		MessageButton pressedButton = MessageButton::Invalid;

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings;

		ImGuiStyle& style = ImGui::GetStyle();

		ImVec2 oldTitleAlign = style.WindowTitleAlign;
		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 center = (vp != nullptr) ? vp->GetCenter() : ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

		float bottomMargin   = 10.0f;
		float minButtonWidth = 80.0f;

		struct ButtonDefinition {
			MessageButton EnumID;
			const char* Label;
		};

		const std::vector<ButtonDefinition> ButtonDefinitions =
		{
			{ MessageButton::Neutral,            "Ok"                  },
			{ MessageButton::SaveAndExit,        "Save And Exit"       },
			{ MessageButton::ExitWithoutSaving,  "Exit Without Saving" },
			{ MessageButton::True,               "Yes"                 },
			{ MessageButton::False,              "No"                  },
			{ MessageButton::Cancel,             "Cancel"              }
		};

		std::vector<ButtonDefinition> VisibleButtons;
		VisibleButtons.reserve(ButtonDefinitions.size());
		for (auto& button : ButtonDefinitions)
			if (HasFlag(buttons, button.EnumID))
				VisibleButtons.push_back(button);

		if (VisibleButtons.empty()) {
			ImGui::End();
			return pressedButton;
		}

		float totalWidth = 0.0f;
		std::vector<float> Widths;
		Widths.reserve(VisibleButtons.size());

		for (auto& button : VisibleButtons) {
			ImVec2 textSize = ImGui::CalcTextSize(button.Label);
			float width = textSize.x + style.FramePadding.x * 2.0f;

			if (width < minButtonWidth)
				width = minButtonWidth;

			Widths.push_back(width);
			totalWidth += width;
		}

		float spacing = style.ItemSpacing.x;
		if (VisibleButtons.size() > 1)
			totalWidth += spacing * (VisibleButtons.size() - 1);

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(totalWidth > 360.0f ? totalWidth + minButtonWidth : 360.0f, 140.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin(fullTitle.c_str(), nullptr, flags);

		style.WindowTitleAlign = oldTitleAlign;

		float buttonFrameHeight = ImGui::GetFrameHeight();
		float footer_reserved = buttonFrameHeight + bottomMargin + style.ItemSpacing.y + style.WindowPadding.y;

		ImGui::BeginChild("##dialog_body", ImVec2(0, -footer_reserved), false, ImGuiWindowFlags_None);

		float regionWidth = ImGui::GetContentRegionAvail().x;

		ImVec2 wrappedSize = ImGui::CalcTextSize(message.c_str(), nullptr, false, regionWidth);

		float textStartX = ImGui::GetCursorPosX() + (regionWidth - wrappedSize.x) * 0.5f;
		if (textStartX < ImGui::GetCursorPosX())
			textStartX = ImGui::GetCursorPosX();

		ImGui::SetCursorPosX(textStartX);
		ImGui::TextWrapped("%s", message.c_str());

		ImGui::EndChild();

		float footerRegionWidth = ImGui::GetContentRegionAvail().x;
		float startX = (footerRegionWidth - totalWidth) * 0.5f;
		if (startX < 0.0f) 
			startX = 0.0f;

		ImGui::SetCursorPosX(startX);

		for (uint32_t i = 0; i < VisibleButtons.size(); i++) {
			if (i > 0)
				ImGui::SameLine(0.0f, spacing);

			ImGui::PushID(i);

			if (ImGui::Button(VisibleButtons[i].Label, ImVec2(Widths[i], 0.0f)))
				pressedButton = VisibleButtons[i].EnumID;

			ImGui::PopID();
		}

		ImGui::End();

		return pressedButton;
	};

	m_Messages.push_back(std::make_pair(MessageUpdate, callback));
}

void MessageManager::PushSound(const std::string& title, const std::string& message, MessageButton buttons, std::function<void(MessageButton)> callback) {
	PromptWarningSound();
	Push(title, message, buttons, callback);
}

constexpr bool MessageManager::HasFlag(MessageButton mask, MessageButton flag) {
	return (uint32_t(mask) & uint32_t(flag)) != 0u;
}
