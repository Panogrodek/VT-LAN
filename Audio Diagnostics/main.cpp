#include "Application.hpp"
#include "tests/InstanceLauncher.hpp"

#include "networking/RPC.hpp"

constexpr bool launchMultipleInstances = 0;
constexpr int childInstances = 1;

int main(int argc, char* argv[]) {
	fs::getRPCMap().Lock();

	{
		//this is needed to properly test networking on localhost
		fs::InstanceConfig cfg{ launchMultipleInstances, childInstances };
		const fs::LaunchMode mode = fs::instanceManager.init(argc, argv, cfg);

		if (mode == fs::LaunchMode::Parent)
			fs::instanceManager.spawnChildrenAsync();
		else if (mode == fs::LaunchMode::Child)
			fs::instanceManager.startParentWatcherIfNeeded();
	}

	fs::WindowData data;
	data.Size = glm::uvec2(1600/2, 900/2);
	data.Title = "Audio Diagnostics";

	fs::Runtime::Initialize(data);

	Application app;
	app.Run();

	return 0;
}
