#include "scheduler.h"
#include "testSystem1.h"
#include "testSystem2.h"
#include "renderSystem.h"


int main() {
	ecs::EntityManager em;
	Scheduler scheduler = Scheduler(em);
	//EntityMover mover = EntityMover(em);
	//EntityPrinter printer = EntityPrinter(em);
	RenderSystem renderer = RenderSystem(em);

	//scheduler.addSystem(&mover);
	//scheduler.addSystem(&printer);
	scheduler.addSystem(&renderer);
	
	// Create some objects
	//auto cmd1 = ecs::EntityBuilder(em).with<Ball>(Ball(10, 12));
	//auto cmd2 = ecs::EntityBuilder(em).with<Ball>(Ball(5, 20));

	/*std::vector<ecs::EntityBuilder>* cmd = new std::vector<ecs::EntityBuilder>;
	cmd->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(10, 12)));
	cmd->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(200, -100)));
	cmd->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(300, -100)));
	cmd->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(300, -100)));*/

	//scheduler.addCommands(cmd);


	for (int i = 0; i < 100; i) {
		scheduler.run();
	}
}