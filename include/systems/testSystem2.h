#pragma once
#include <cstdio>
#include "systemBase.h"
#include "Components.h"

class EntityPrinter : public ISystem {
private:
	double gravity = 10;
	ecs::EntityManager& em;
	ecs::ECSView<Ball> view;
public:
	EntityPrinter(ecs::EntityManager& manager) : em(manager), view(&em){}
	std::vector<ecs::EntityBuilder>* run(uint32_t dt) {
		std::vector<ecs::EntityBuilder>* commands = new std::vector<ecs::EntityBuilder>;

		int ballCount = 0;
		for (auto& [ball] : view) {
			std::cout << "Position of Ball " << ballCount << " is " << ball.x << "\n";
			std::cout << "Memory location is " << &ball.x << "\n";
			ballCount++;
		}

		return commands;
	}

	void updateView() {
		view.update();
	}
};