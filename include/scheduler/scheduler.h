#include <vector>
// #include <bitset>
// #include <functional>
// #include "config.h"
#include "systemBase.h"

extern const size_t MAX_COMPONENTS;

class Scheduler {
public:
	Scheduler(ecs::EntityManager& manager) : em(manager) {}
	ecs::EntityManager& em;
	std::vector<ISystem*> systems;
	std::vector<std::vector<ecs::EntityBuilder>*> commandQueue;
	int dt = 100;

	void run() {
		flush();
		for (auto& sys : systems) {
			sys->updateView();
			commandQueue.push_back(sys->run(dt));	// Take ownership of commandQueue
		}
	}

	void addSystem(ISystem* sys) {
		systems.push_back(sys);
	}

	void addCommands(std::vector<ecs::EntityBuilder>* commands) {
		commandQueue.push_back(commands);
	}

private:
	void flush() {
		int i = 0;
		for (auto& commandVec : commandQueue) {
			for (auto& command : *commandVec) {
				command.build();
			}
			delete commandVec;
		}
		commandQueue.clear();
	}
};