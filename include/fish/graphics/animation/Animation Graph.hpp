#pragma once

#include <initializer_list>
#include <unordered_map>

#include "Animation.hpp"

namespace fs {
	struct AnimationNode {
	public:
		AnimationNode()  = default;
		~AnimationNode() = default;

		template<typename T>
		AnimationNode(struct Animation& animation, T state)
			: Animation(animation)
			, State((uint32_t)state)
		{ }

		struct Animation Animation;
		uint32_t State = 0; // 0 is entry state
	};

	class AnimationGraph {
	public:
		AnimationGraph()  = default;
		~AnimationGraph() = default;
		
		template<typename T>
		void SetState(T state, bool keepFrame = false) {
			uint32_t lastState = m_state;
			m_state = (uint32_t)state;

			if (lastState != m_state) {
				//m_AnimationNodes[m_state].Animation.SetFrame(!keepFrame ? 0 : m_AnimationNodes[lastState].Animation.CurrentFrame); // idk about that tho
				m_AnimationNodes[m_state].Animation.ElapsedTime = !keepFrame ? 0.0f : m_AnimationNodes[lastState].Animation.ElapsedTime;
			}
		}

		void SetBody(fs::Quad& quad);

		void AddAnimations(std::initializer_list<AnimationNode> Animations);
		void AddAnimations(AnimationNode animation);

		uint32_t GetState() const;

		// returns all animations of graph that pass given condition
		// heavy function shouldn't be used every frame
		template<typename Func, typename... Args>
		std::vector<Animation*> GetAnimationsConditional(Func Condition, Args&&... args) {
			std::vector<Animation*> Animations;

			for (auto& node : m_AnimationNodes)
				if (Condition(node.second, std::forward<Args>(args)...))
					Animations.push_back(&node.second.Animation);

			return Animations;
		}

		Animation& GetCurrentAnimation();
		Animation& GetAnimation(uint32_t state);

		Animation& operator[](uint32_t state);

	private:
		std::unordered_map<uint32_t, AnimationNode> m_AnimationNodes;

		uint32_t m_state = 0;
	};
}
