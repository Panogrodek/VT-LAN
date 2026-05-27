#include "states/State Machine.hpp"

using namespace priv;

void State::Shutdown() {
	m_running = false;
}

void StateMachine::Update() {
	if (m_States.empty())
		return;

	auto& state = m_States.front();

	if (!state->m_running) {
		delete state;
		m_States.pop_front();

		return;
	}

	state->Update();
}

void StateMachine::Render() {
	if (m_States.empty())
		return;

	m_States.front()->Render();
}

void StateMachine::PhysicsUpdate()
{
	if (m_States.empty())
		return;

	m_States.front()->PhysicsUpdate();
}

void StateMachine::PushTop(State* state) {
	m_States.push_front(state);
}

void StateMachine::PushBottom(State* state) {
	m_States.push_back(state);
}

void StateMachine::PopTop() {
	delete m_States.front();
	m_States.pop_front();
}

void StateMachine::PopBottom() {
	delete m_States.back();
	m_States.pop_back();
}

void StateMachine::Clear() {
	while (!m_States.empty()) {
		delete m_States.back();
		m_States.pop_back();
	}
}

State* StateMachine::GetTop() {
	if (m_States.empty())
		return nullptr;

	return m_States.front();
}

std::deque<State*>& StateMachine::GetStates() {
	return m_States;
}
