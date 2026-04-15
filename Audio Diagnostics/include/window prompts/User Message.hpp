#pragma once

#include <unordered_map>
#include <functional>
#include <string>

enum class MessageButton : uint32_t {
	Invalid			  = 0,
	Neutral			  = 1 << 0,
	SaveAndExit		  = 1 << 1,
	ExitWithoutSaving = 1 << 2,
	True			  = 1 << 3,
	False			  = 1 << 4,
	Cancel			  = 1 << 5,
};

inline MessageButton operator|(MessageButton a, MessageButton b) {
	return MessageButton((uint32_t)a | (uint32_t)b);
}

inline MessageButton& operator|=(MessageButton& a, MessageButton b) {
	a = MessageButton((uint32_t)a | (uint32_t)b);
	return a;
}

inline MessageButton operator&(MessageButton a, MessageButton b) {
	return MessageButton((uint32_t)a & (uint32_t)b);
}

namespace priv {
	class MessageManager {
	public:
		MessageManager()  = default;
		~MessageManager() = default;

		void Update();
		void Push(const std::string& title, const std::string& message, MessageButton buttons = MessageButton::Neutral, std::function<void(MessageButton)> callback = 0);
		void PushSound(const std::string& title, const std::string& message, MessageButton buttons = MessageButton::Neutral, std::function<void(MessageButton)> callback = 0);

	private:
		std::vector<std::pair<std::function<MessageButton()>, std::function<void(MessageButton)>>> m_Messages; // yeah this is easiest too
		std::unordered_map<std::string, uint32_t> m_TitleCounter; // dumb but easiest

		static constexpr bool HasFlag(MessageButton mask, MessageButton flag);
	};
}

inline priv::MessageManager MessageManager;
