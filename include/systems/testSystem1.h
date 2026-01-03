#pragma once
#include "systemBase.h"
#include "ECSView.h"
#include "Components.h"

class EntityMover : public ISystem {
private:
	double gravity = 10;
	ecs::EntityManager& em;
	ecs::ECSView<Ball> view;
public:
	EntityMover(ecs::EntityManager& manager) : em(manager), view(&em) {}
	
	std::vector<ecs::EntityBuilder>* run(uint32_t dt) {
		std::vector<ecs::EntityBuilder>* commands = new std::vector<ecs::EntityBuilder>;

		for (auto& [ball] : view) {
			ball.x += ball.vel * dt;
		}

		commands->push_back(ecs::EntityBuilder(em).with<Ball>(Ball(10, 12)));

		return commands;
	}

	void updateView() {
		view.update();
	}
};