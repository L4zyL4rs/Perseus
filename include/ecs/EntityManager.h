#pragma once
#include <unordered_map>
#include <bitset>
#include <assert.h>
#include <algorithm>
#include <type_traits>
#include <iostream>
//#include "Components.h"
#include "Registry.h"

namespace ecs {

	// This should suffice for basically everything
	/*const size_t MAX_COMPONENTS = 256;*/

	const std::bitset<MAX_COMPONENTS> NULLSET = 0;

	struct EntityMeta {
		Archetype* pArchetype = nullptr;
		uint32_t indexInArchetype = UINT32_MAX;
	};

	class EntityManager {
	public:
		std::vector<EntityMeta> entityMetaVector;
		std::unordered_map<std::bitset<MAX_COMPONENTS>, Archetype> archetypes;
		IndexQueue entityQueue;
		std::unordered_map<std::bitset<MAX_COMPONENTS>, std::vector<Archetype*>> systemRegistry;		// DO NOT REALLOCATE WITHOUT CHANGING ECSVIEW

		Entity createEntity() {
			std::bitset<MAX_COMPONENTS> components = NULLSET;

			Entity entity = entityQueue.generateID();
			std::cout << "Creating entity " << entity << "\n";
			assert(entity <= entityMetaVector.size());


			EntityMeta entityMeta;


			if (entity < entityMetaVector.size()) {
				entityMetaVector[entity] = entityMeta;
			}
			else {
				entityMetaVector.push_back(entityMeta);
			}

			migrateEntity(entity, components, NULLSET);

			return entity;
		}

		// THIS ASSUMES THAT COMPONENT HAS ALREADY BEEN REGISTERED
		// Should not happen, someone needs to get the ComponentType from the registry
		void addComponent(Entity entity, ComponentType type, void* data) {
			assert(gComponentRegistry.size() > type);
			if (hasComponent(entity, type)) { return; }

			std::bitset<MAX_COMPONENTS> oldComponents = entityMetaVector[entity].pArchetype->components;

			std::bitset<MAX_COMPONENTS> newComponents = oldComponents;
			newComponents[type] = true;

			createArchetype(newComponents);
			migrateEntity(entity, newComponents, oldComponents);

			copyComponent(type, entityMetaVector[entity].pArchetype->components, entityMetaVector[entity].indexInArchetype, data);
			std::cout << "Added component of type " << type << "\n";
		}

		// Migrates ONLY MATCHING components from one archetype to another
		// Some of this code seems weird because I want to maybe change defragmentation later
		void migrateEntity(Entity entity, std::bitset<MAX_COMPONENTS> dst, std::bitset<MAX_COMPONENTS> src) {
			Entity currentIndex = entityMetaVector[entity].indexInArchetype;
			Entity newIndex = archetypes[dst].storageQueue.generateID();
			if(currentIndex != UINT32_MAX) {
				// Only do the whole copying if the entity has actually been registered somewhere
				copyComponents(dst, newIndex, src, entityMetaVector[entity].indexInArchetype);

				if (archetypes[src].indexToEntity.size() > 1) {
					Entity defragIndex = archetypes[src].storageQueue.highestID;
					Entity defragEntity = archetypes[src].indexToEntity[defragIndex];
					copyComponents(src, currentIndex, src, defragIndex);
					archetypes[src].indexToEntity[currentIndex] = defragEntity;
					entityMetaVector[defragEntity].indexInArchetype = currentIndex;
				}

				if (archetypes[src].indexToEntity.size() > 0) {
					archetypes[src].indexToEntity.pop_back();
					archetypes[src].storageQueue.freeID(currentIndex);
				}
			}
 
			entityMetaVector[entity].indexInArchetype = newIndex;
			if (archetypes[dst].indexToEntity.size() == newIndex) {
				archetypes[dst].indexToEntity.push_back(entity);
			}
			else {
				archetypes[dst].indexToEntity[newIndex] = entity;
			}
			entityMetaVector[entity].pArchetype = &archetypes[dst];
		}

		// Copies ONLY MATCHING components from one archetype to another
		void copyComponents(std::bitset<MAX_COMPONENTS> dst, uint32_t dstIndex, std::bitset<MAX_COMPONENTS> src, uint32_t srcIndex) {
			std::bitset<MAX_COMPONENTS> matchingComponents = dst & src;
			for (int i = 0; i < MAX_COMPONENTS; i++) {
				if (matchingComponents[i]) {
					copyComponent(i, dst, dstIndex, src, srcIndex);
				}
			}
		}

		void copyComponent(ComponentType component, std::bitset<MAX_COMPONENTS> dst, uint32_t dstIndex, std::bitset<MAX_COMPONENTS> src, uint32_t srcIndex) {
			archetypes[dst].storages[component].insert((char*)archetypes[src].storages[component].getEntryP(srcIndex), dstIndex);
		}

		void copyComponent(ComponentType component, std::bitset<MAX_COMPONENTS> dst, uint32_t dstIndex, void* srcData) {
			archetypes[dst].storages[component].insert(srcData, dstIndex);
		}

		void removeComponent(Entity entity, ComponentType type) {
			assert(gComponentRegistry.size() > type);
			if (!hasComponent(entity, type)) { return; }

			std::bitset<MAX_COMPONENTS> oldComponents = entityMetaVector[entity].pArchetype->components;

			std::bitset<MAX_COMPONENTS> newComponents = oldComponents;
			newComponents[type] = false;

			createArchetype(newComponents);
			migrateEntity(entity, newComponents, oldComponents);
		}

		Archetype* createArchetype(std::bitset<MAX_COMPONENTS> components) {
			if (!archetypes.contains(components)) {
				std::cout << "Created archetype for " << components << "\n";
				archetypes[components].components = components;

				for (int i = 0; i < MAX_COMPONENTS; i++) {
					// Populate the components for the new Archetype
					if (components[i]) {
						archetypes[components].storages.push_back(ComponentStorage(gComponentRegistry[i].size));
					}

				}

				// I think this makes sense?
				for (auto& [systemComponents, vector] : systemRegistry) {
					if ((systemComponents & components) == systemComponents) {
						vector.push_back(&archetypes[components]);
					}
				}
			}

			// I think this makes sense?
			/*for (auto& [systemComponents, vector] : systemRegistry) {
				if ((systemComponents & components) == systemComponents) {
					vector.push_back(&archetypes[components]);
				}
			}*/

			return &archetypes[components];
		}

		// Removes entity data from its current Archetype and self defrags the component storage
		void removeEntity(Entity ID) {
			std::bitset<MAX_COMPONENTS> components = entityMetaVector[ID].pArchetype->components;

			if (entityMetaVector[ID].indexInArchetype == entityMetaVector[ID].pArchetype->storageQueue.highestID) {
				entityMetaVector[ID].pArchetype->storageQueue.freeID(entityMetaVector[ID].indexInArchetype);
				entityMetaVector[ID].pArchetype = nullptr;
				return;
			}

			copyComponents(components, entityMetaVector[ID].indexInArchetype, components, entityMetaVector[ID].pArchetype->storageQueue.highestID);

			entityMetaVector[ID].pArchetype->storageQueue.freeID(entityMetaVector[ID].indexInArchetype);
			entityMetaVector[ID].pArchetype = nullptr;
		}

		bool hasComponent(Entity entity, ComponentType type) {
			return entityMetaVector[entity].pArchetype->components[type];
		}


		std::vector<Archetype*> archetypesForSystem(std::bitset<MAX_COMPONENTS> system) {
			std::vector<Archetype*> matchingArchetypes;
			for (auto& [components, archetype] : archetypes) {
				if ((components & system) == system) {
					std::cout << "Matching Archetype " << components << "\n";
					matchingArchetypes.push_back(&archetype);
				}
			}
			return matchingArchetypes;
		}

		void registerSystem(std::bitset<MAX_COMPONENTS> components) {
			systemRegistry[components] = archetypesForSystem(components);
		}

		bool systemIsRegistered(std::bitset<MAX_COMPONENTS> components) {
			return systemRegistry.contains(components);
		}
	};

}