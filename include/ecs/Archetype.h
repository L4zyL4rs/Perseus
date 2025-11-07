#pragma once
#include<bitset>
#include<vector>
#include "ComponentStorage.h"
#include "IndexQueue.h"
#include "config.h"

extern const size_t MAX_COMPONENTS;

namespace ecs {
	using ComponentType = uint32_t;

	struct Archetype {
		std::bitset<MAX_COMPONENTS> components;
		std::vector<Entity> indexToEntity;
		std::vector<ComponentStorage> storages;
		IndexQueue storageQueue;

		uint32_t findComponentIndex(ComponentType t) {
			uint32_t idx = 0;
			for (int i = 0; i < t; i++) {
				if (components[idx]) { idx++; }
			}
			return idx;
		}
	};
}