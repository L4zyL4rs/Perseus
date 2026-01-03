#pragma once
//#include <unordered_map>
#include "EntityManager.h"


// API shuld be usable in the pattern 
// auto e = ecs::EntityBuilder(em).with<Transform>(trans).with<Velocity>(vel).build();

namespace ecs {

	class EntityBuilder {
	public:
		explicit EntityBuilder(EntityManager& em) : em_(em) { std::cout << "Built command for entity " << e_ << "\n"; }

		EntityBuilder& forEntity(Entity e) {
			e_ = e;
			return *this;
		}

		template <typename T>
		EntityBuilder& with(const T& comp) {
			Holder<T>* h = new Holder<T>(comp);
			adds_.push_back(h);
			return *this;
		}

		template <typename T>
		EntityBuilder& without() {
			removes_.push_back(ecs::getComponentID<T>());

			return *this;
		}

		Entity build() {
			std::cout << "Building entity " << e_ << "\n";
			if (e_ == UINT32_MAX) {
				e_ = em_.createEntity();
			}

			for (auto& a : adds_) {
				a->apply(em_, e_);
				delete a;
			}

			for (auto& r : removes_) {
				em_.removeComponent(e_, r);
			}

			adds_.clear();
			removes_.clear();

			std::cout << "Successfully built entity " << e_;
			
			return e_;
		}

	private:
		struct IHolder {
			virtual ~IHolder() = default;
			virtual void apply(EntityManager&, Entity) = 0;
		};

		template <typename T>
		struct Holder : IHolder {
			T comp;
			explicit Holder(const T& c) : comp(c) {};
			void apply(EntityManager& em, Entity e) override { em.addComponent(e, ecs::getComponentID<T>(), &comp); }
		};

		EntityManager& em_;
		Entity e_ = UINT32_MAX;
		std::vector<IHolder*> adds_;
		std::vector<ComponentType> removes_;
};

}




//
//namespace ecs {
//	// Packaging data so that scheduler can write it to ECS
//	// ALL components MUST be set in ONE command before writing
//	class EntityBuilder {
//	public:
//		int setComponents(std::bitset<ecs::MAX_COMPONENTS> newComponents) {
//			if (!componentsSet) {
//				components = newComponents;
//				size_t offset = 0;
//				for (int i = 0; i < ecs::MAX_COMPONENTS; i++) {
//					if (components[i]) {
//						componentOffsets[i] = offset;
//						offset += ecs::gComponentRegistry[i].size;
//					}
//				}
//				size = offset;
//				data = malloc(size);
//				componentsSet = true;
//				return 0;
//			}
//			return 1;
//		}
//
//		// Can only be done AFTER setComponents has been called
//		int setRemoveComponents(std::bitset<ecs::MAX_COMPONENTS> toRemove) {
//			if (!componentsSet) { return 1; }
//
//			std::bitset<ecs::MAX_COMPONENTS> check = toRemove;
//			check &= components;
//
//			if (check != ecs::NULLSET) { return 1; }
//
//			removeComponents = toRemove;
//			return 0;
//		}
//		
//		int setEntity(ecs::Entity e) {
//			if (entity != UINT32_MAX) { return 1; }
//			entity = e;
//			return 0;
//		}
//
//		template<typename T>
//		int writeComponent(T* src) {
//			if (!componentsSet) { return 1; }
//
//			ecs::ComponentType type = ecs::getComponentID<T>();
//			void* dst = (void*)((char*)data + componentOffsets[type]);
//
//			memcpy(dst, &src, ecs::gComponentRegistry[type].size);
//			componentsWritten[type] = true;
//			return 0;
//		}
//
//		// DELETES THIS INSTANCE
//		bool flush(ecs::EntityManager* em) {
//			if (!(componentsWritten == components)) { return false; }
//			if (entity == UINT32_MAX) { 
//				entity = em->createEntity(); 
//			}
//
//			for (int i = 0; i < ecs::MAX_COMPONENTS; i++) {
//				if (components[i]) {
//					em->addComponent(entity, i, (char*)data + componentOffsets[i]);
//				}
//				if (removeComponents[i]) {
//					em->removeComponent(entity, i);
//				}
//			}
//
//			free(data);
//			delete this;
//			return true;
//		}
//
//	private:
//		ecs::Entity entity = UINT32_MAX;
//		std::bitset<ecs::MAX_COMPONENTS> components;
//		std::bitset<ecs::MAX_COMPONENTS> removeComponents = ecs::NULLSET;
//		std::bitset<ecs::MAX_COMPONENTS> componentsWritten = ecs::NULLSET;
//		bool componentsSet = false;
//		size_t size;
//		std::unordered_map<ecs::ComponentType, size_t> componentOffsets;
//		void* data = nullptr;
//	};
//}