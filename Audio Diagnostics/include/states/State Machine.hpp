#pragma once

#include <deque>

namespace priv {
	class StateMachine;
}

class State {
public:
	State() = default;
	virtual ~State() = default;

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void PhysicsUpdate() {}; //ig that we dont need physics update in every state

	void Shutdown();

protected:
	bool m_running = true;

	friend class priv::StateMachine;
};

namespace priv {
	class StateMachine {
	public:
		StateMachine() = default;
		~StateMachine() = default;

		void Update();
		void Render();
		void PhysicsUpdate();

		void PushTop(State* state);
		void PushBottom(State* state);

		void PopTop();
		void PopBottom();

		void Clear();

		State* GetTop();

		std::deque<State*>& GetStates();

	private:
		std::deque<State*> m_States;
	};
}

inline priv::StateMachine StateMachine;
