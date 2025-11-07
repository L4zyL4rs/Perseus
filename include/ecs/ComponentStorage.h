#pragma once
#include <cstdint>
#include <stdlib.h>
#include <cstring>

// Universal container for any component data
// Layout of memory is managed by EntityManager through EntityMeta.indexInArchetype

namespace ecs {
	inline constexpr uint32_t MINIMUM_RESERVATION = 10;

	class ComponentStorage {
	public:
		void* data;
		size_t componentSize;

		// Reserved and stored number of entities
		// For now, all reserved memory is valid
		uint32_t reserved;		

		ComponentStorage(size_t size) {
			componentSize = size;
			reserved = MINIMUM_RESERVATION;

			data = malloc(componentSize * MINIMUM_RESERVATION);
		}

		void insert(void* source, uint32_t index) {
			if (index >= reserved) {
				void* biggerContainer = malloc(componentSize * reserved * 2);
				std::memcpy(biggerContainer, data, reserved);
				reserved *= 2;
				free(data);
				data = biggerContainer;
			}
			memcpy(getEntryP(index), source, componentSize);
			return;
		}

		void* getEntryP(uint32_t index) {
			return (char*)data + index * componentSize;
		}
	};

}