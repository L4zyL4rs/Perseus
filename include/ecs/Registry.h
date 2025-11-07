#pragma once
#include <string>
#include <vector>
#include <bitset>
#include "Archetype.h"

namespace ecs {
	using SystemType = uint32_t;

	struct ComponentInfo {
		std::string name;
		size_t size = 0;
		size_t alignment = 0;	// Still not used anywhere for now, TODO
	};

	inline std::vector<ComponentInfo> gComponentRegistry;

	inline ComponentType getNextComponentID() {
		static ComponentType nextID = 0;
		nextID++;
		return nextID - 1;
	}

	template<typename T>
	ComponentType getComponentID() {
		// Using trickery with static keywords and templates
		// For any type T, componentID gets initialised only once
		static ComponentType componentID = getNextComponentID();
		static bool registered = [] {
			ComponentInfo info;
			info.name = typeid(T).name();
			info.size = sizeof(T);
			info.alignment = alignof(T);
			gComponentRegistry.push_back(info);
			return true;
			} ();
		return componentID;
	}
};