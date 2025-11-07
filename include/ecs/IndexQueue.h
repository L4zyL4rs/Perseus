#pragma once
#include<vector>
#include "Entity.h"

namespace ecs {
	// Handling entity IDs
		// freedIDs and freeID() might be confusing
	class IndexQueue {
	public:
		Entity highestID = UINT32_MAX;
		std::vector<Entity> freedIDs;

		Entity generateID() {
			if (freedIDs.empty()) {
				highestID++;
				return highestID;
			}
			else {
				Entity ID = freedIDs.back();
				freedIDs.pop_back();
				return ID;
			}
		}

		void freeID(Entity entity) {
			// Avoid freeing the same ID twice
			for (auto& f : freedIDs) {
				if (f == entity) { return; }
			}
			if (entity == highestID) { 
				highestID--; 
				return;
			}
			freedIDs.push_back(entity);
			return;
		}
	};
}