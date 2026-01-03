#pragma once
//#include <bitset>
#include <vector>
#include "EntityBuilder.h"
//#include "ECSView.h"

extern const size_t MAX_COMPONENTS;

class ISystem {
public:
	~ISystem() = default;

	virtual void updateView() = 0;
	virtual std::vector<ecs::EntityBuilder>* run(uint32_t dt) = 0;
};