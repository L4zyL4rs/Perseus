#include "EntityBuilder.h"
#include "EntityManager.h"
#include "scheduler.h"
#include "testSystem1.h"
#include "testSystem2.h"
#include "renderSystem.h"
#include <chrono>
#include <thread>
#include <chrono>
#include <thread>

std::vector<ecs::EntityBuilder> startScene(ecs::EntityManager& em, RenderSystem& rend);

int main() {
	ecs::EntityManager em;
	Scheduler scheduler = Scheduler(em);
	//EntityMover mover = EntityMover(em);
	//EntityPrinter printer = EntityPrinter(em);
	RenderSystem renderer = RenderSystem(em, 1600, 1200);

	//scheduler.addSystem(&mover);
	//scheduler.addSystem(&printer);
	scheduler.addSystem(&renderer);
	
	// Create some objects
	//auto cmd1 = ecs::EntityBuilder(em).with<Ball>(Ball(10, 12));
	//auto cmd2 = ecs::EntityBuilder(em).with<Ball>(Ball(5, 20));

	std::vector<ecs::EntityBuilder>* cmd = new std::vector<ecs::EntityBuilder>;
	cmd->append_range(startScene(em, renderer));
    cmd->append_range(renderer.init());
    /*cmd->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(10, 12)));
	cmd->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(200, -100)));
	cmd->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(300, -100)));
	cmd->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(300, -100)));*/

	scheduler.addCommands(cmd);

    ecs::ECSView<EngineControl> controlView(&em);

	for (int i = 0; i < 100; i) {
		scheduler.run();
        controlView.update();
        auto itControl = controlView.begin();
        auto& [ctrl] = *itControl;
        if(ctrl.requestExit) { break; }
	    //std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

std::vector<ecs::EntityBuilder> startScene(ecs::EntityManager& em, RenderSystem& rend) {
    uint32_t WINDOWWIDTH = 1600;
    uint32_t WINDOWHEIGHT = 1200;

    std::string sphere = "sphere";
    std::string cube = "cube";
    std::string robotoslab = "robotoslab";
    std::string text = "Hello World";
    std::vector<ecs::EntityBuilder> scene;
    scene.emplace_back(ecs::EntityBuilder(em).with<RenderObject>(RenderObject(glm::vec3(1), glm::vec3(0), rend.frameManager.assetManager.loadAsset(sphere))));
    scene.emplace_back(ecs::EntityBuilder(em).with<RenderObject>(RenderObject(glm::vec3(-1), glm::vec3(0), rend.frameManager.assetManager.loadAsset(cube))));

    scene.emplace_back(ecs::EntityBuilder(em).with<TextElement128>(TextElement128(text, rend.frameManager.assetManager.loadFont(robotoslab, 30), glm::vec2(0.2, 0.2), glm::vec4(1))));
    scene.emplace_back(ecs::EntityBuilder(em).with<UserInput>(UserInput()));

    Camera cam;
    cam.lastX = WINDOWWIDTH / 2;
    cam.lastY = WINDOWHEIGHT / 2;
    scene.emplace_back(ecs::EntityBuilder(em).with<Camera>(cam));
    scene.emplace_back(ecs::EntityBuilder(em).with<EngineControl>(EngineControl()));
    return scene;
}
