#pragma once

#include "Animation Graph.hpp"

namespace fs {
	namespace fs_priv {
		class Animator {
		public:
			Animator()  = default;
			~Animator() = default;

			void Play(AnimationGraph& graph);
			
		private:
			void UpdateAnimation(Animation& animation);
			void UpdateAnimationReverse(Animation& animation);
		};
	}

	inline fs_priv::Animator Animator;
}
