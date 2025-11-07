#include <cassert>
#include <iostream>
#include "ECSView.h"

// This has been blatantly vibecoded, but tests the necessary functions

namespace ecs {
    // Dummy components
    struct dummyComponentA { int x; };
    struct dummyComponentB { int y, z; };
}

//int main() {
//    using namespace ecs;
//
//    EntityManager em;
//
//    // === Register components ===
//    ComponentType idA = getComponentID<dummyComponentA>();
//    ComponentType idB = getComponentID<dummyComponentB>();
//    assert(idA != idB);
//    std::cout << "Registered components A=" << idA << ", B=" << idB << "\n";
//
//    // === Create entities and add components ===
//    std::bitset<MAX_COMPONENTS> signatureA;
//    signatureA[idA] = true;
//
//    std::bitset<MAX_COMPONENTS> signatureAB;
//    signatureAB[idA] = true;
//    signatureAB[idB] = true;
//
//    Entity e1 = em.createEntity();   // only A
//    Entity e2 = em.createEntity();  // A + B
//
//    dummyComponentA compA1{ 10 };
//    dummyComponentA compA2{ 20 };
//    dummyComponentB compB2{ 30, 40 };
//
//    em.addComponent(e1, idA, &compA1);
//    em.addComponent(e2, idA, &compA2);
//    em.addComponent(e2, idB, &compB2);
//
//    assert(em.hasComponent(e1, idA));
//    assert(em.hasComponent(e2, idA));
//    assert(em.hasComponent(e2, idB));
//    std::cout << "Entities with components set up correctly\n";
//}

int main() {
    using namespace ecs;

    EntityManager em;

    // === Component registration test ===
    ComponentType idA = getComponentID<dummyComponentA>();
    ComponentType idB = getComponentID<dummyComponentB>();

    assert(idA != idB); // IDs should be unique
    std::cout << "Component IDs: A=" << idA << ", B=" << idB << "\n";
    std::cout << "Registry size: " << gComponentRegistry.size() << "\n";
    assert(gComponentRegistry.size() == 2);

    // === Entity creation with builder ===
    dummyComponentA compA{ 42 };
    dummyComponentB compB{ 7, 8 };

    Entity e1 = EntityBuilder(em)
        .with(compA)
        .with(compB)
        .build();

    std::cout << "Entity built: " << e1 << " with A+B\n";

    assert(em.hasComponent(e1, idA));
    assert(em.hasComponent(e1, idB));

    // === Create another entity ===
    dummyComponentA compA2{ 99 };
    Entity e2 = EntityBuilder(em)
        .with(compA2)
        .build();

    std::cout << "Entity built: " << e2 << " with A only\n";
    assert(em.hasComponent(e2, idA));
    assert(!em.hasComponent(e2, idB));

    // === Remove a component using builder ===
    EntityBuilder(em)
        .forEntity(e1)
        .without<dummyComponentB>()
        .build();

    std::cout << "Removed component B from entity " << e1 << "\n";
    assert(!em.hasComponent(e1, idB));

    // === Archetype query test ===
    std::bitset<MAX_COMPONENTS> system;
    system[idA] = true;

    auto archetypesWithA = em.archetypesForSystem(system);
    assert(!archetypesWithA.empty());
    std::cout << "System query with A found " << archetypesWithA.size() << " archetypes\n";

    // === Data verification ===
    for (auto& archetype : archetypesWithA) {
        if (archetype->storageQueue.highestID == UINT32_MAX)
            continue;

        for (int i = 0; i <= archetype->storageQueue.highestID; i++) {
            auto* aPtr = static_cast<dummyComponentA*>(archetype->storages[idA].getEntryP(i));
            if (aPtr) {
                std::cout << "Data for component A : " << aPtr->x << "\n";
            }
        }
    }

    // === Optional: iterator test ===
    std::cout << "\n=== Iterator test ===\n";
    em.registerSystem(system);
    ECSView<dummyComponentA> view(&em, system);
    for (auto it = view.begin(); it.archetypeIndex < view.end().archetypeIndex; it++) {
        auto [a] = *it;
        std::cout << "Iterated A = " << a.x << "\n";
    }

    std::cout << "\n All ECS tests passed!\n";
    return 0;
}