#pragma once

namespace fs {
	
	struct InstanceConfig {
		bool enableMultiInstance = true;
		int  childCount = 1;
	};

	enum class LaunchMode {
		Single,  // no special multi-instance behavior
		Parent,  // this process is the parent (spawns children)
		Child    // this process is a child (monitors parent)
	};

	namespace fs_priv {

		class InstanceManager {
		public:
			InstanceManager() = default;

			// Decide mode based on args and config; may spawn children if Parent.
			LaunchMode init(int argc, char* argv[], const InstanceConfig& cfg);

			// For Child mode: start async parent watcher if parentPID is set.
			void startParentWatcherIfNeeded();

			// Should be called in Parent mode to spawn N children asynchronously.
			void spawnChildrenAsync();

			// Returns parent PID parsed from args when in Child mode (0 if none).
			std::uint32_t parentPid() const { return m_parentPid; }

		private:
			InstanceConfig m_cfg;
			LaunchMode     m_mode = LaunchMode::Single;
			std::uint32_t  m_parentPid = 0;

			void spawnOneChild();
			void parseParentPidFromArgs(int argc, char* argv[]);
		};
	}
	inline fs_priv::InstanceManager instanceManager;
}