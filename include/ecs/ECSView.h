#pragma once
#include <iterator>
#include <cstddef>
#include <tuple>
#include <bitset>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <stdexcept>
#include <array>
#include "EntityBuilder.h"
#include "Meta.h"

namespace ecs {

	template<typename... Ts>
	class ECSView {
	public:
		// Convenience
		using pack_t = std::tuple<Ts...>;
		//using ComponentTupleRef = std::tuple<Ts&...>;
		//using ComponentTuplePtr = std::tuple<Ts*...>;
		static constexpr std::size_t N = sizeof...(Ts);

		static_assert(N > 0, "ECSView requires at least one component type");

		ECSView(EntityManager* entityManager) {
			em = entityManager;
			writeSignature();
			em->registerSystem(systemSignature);

			// MIGHT BECOME DANGLING
			archetypes = &(em->systemRegistry[systemSignature]);

			update();
		}

		// Let scheduler update the data in ECSView
		void update() {
			version++;

			uint32_t numberOfArchetypes = (*archetypes).size();
			componentIdx.update(archetypes);
			componentStarts.update(archetypes, componentIdx);
			storageSizes.update(archetypes);
			//std::cout << "Archetypes size: " << archetypes->size() << "\n";
		}

		// Iterator representing the CURRENT state of the ECSView
		// Will be invalid when ECSView is updated
		struct Iterator {
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = std::tuple<Ts...>;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = std::tuple<Ts&...>;

			ECSView* view = nullptr;
			uint32_t currentEntity = UINT32_MAX;
			uint32_t archetypeIndex = UINT32_MAX;
			uint32_t iteratorVersion = UINT32_MAX;

			bool checkValid() {
				if (view->version != iteratorVersion) {
					throw std::runtime_error("Iterator invalidated!");
				}
				return true;
			}

			const reference operator*() {
				checkValid();
				return std::tuple<Ts&...> {
					*static_cast<Ts*>(view->getPointer<Ts>(ecs::getComponentID<Ts>(), currentEntity, archetypeIndex))...
				};
			}

			// Skip this one for now, probably not needed
			/*pointer operator->() {
				checkValid();
				
			}*/

			Iterator operator++(int) {
				Iterator old = *this;
				currentEntity++;
				/*if (currentEntity >= view->storageSizes[archetypeIndex]) {
					currentEntity = 0;
					archetypeIndex++;
				}*/

				if (currentEntity < view->storageSizes[archetypeIndex]) {
					return old;
				}

				do {
					archetypeIndex++;
					currentEntity = 0;
				} while (archetypeIndex < view->archetypes->size() && view->storageSizes[archetypeIndex] == UINT32_MAX);
				return old;
			}

			Iterator operator++() {
				//std::cout << "Starting with " << *this << "For archetype size " << view->archetypes->size() << "\n";
				currentEntity++;

				if (currentEntity <= view->storageSizes[archetypeIndex]) {
					return *this;
				}

				/*if (currentEntity >= view->storageSizes[archetypeIndex]) {
					currentEntity = 0;
					archetypeIndex++;
				}*/

				do {
					archetypeIndex++;
					currentEntity = 0;
				} while (archetypeIndex < view->archetypes->size() && view->storageSizes[archetypeIndex] == UINT32_MAX);

				//std::cout << "incremented to " << *this;
				return *this;
			}

			Iterator operator--(int) {
				Iterator old = *this;
				currentEntity--;
				if (currentEntity == UINT32_MAX) {
					currentEntity = 0;
					archetypeIndex--;
					if (archetypeIndex == UINT32_MAX) {
						throw std::runtime_error("Iterator decremented past begin()");
					}
				}
				return old;
			}

			Iterator operator--() {
				currentEntity--;
				if (currentEntity == UINT32_MAX) {
					currentEntity = 0;
					archetypeIndex--;
					if (archetypeIndex == UINT32_MAX) {
						throw std::runtime_error("Iterator decremented past begin()");
					}
				}
				return *this;
			}

			inline bool operator==(const Iterator& rhs) {
				//std::cout << "Comparing iterators\n" << *this << rhs;
				return (iteratorVersion == rhs.iteratorVersion) && (archetypeIndex == rhs.archetypeIndex) && (currentEntity == rhs.currentEntity);
			}

			inline bool operator!=(const Iterator& rhs) {
				return !(*this == rhs);
			}

			friend std::ostream& operator<<(std::ostream& out, const Iterator& it) {
				return out
					<< "Iterator:\n"
					<< "  currentEntity: " << it.currentEntity << "\n"
					<< "  archetypeIndex: " << it.archetypeIndex << "\n"
					<< "  iteratorVersion: " << it.iteratorVersion << "\n";
			}
		};


		Iterator begin() {
			Iterator it;
			it.view = this;
			it.currentEntity = 0;
			it.archetypeIndex = 0;
			it.iteratorVersion = this->version;

			return it;
		}

		Iterator end() {
			Iterator it;
			it.view = this;
			it.currentEntity = 0;
			it.archetypeIndex = archetypes->size();
			it.iteratorVersion = this->version;

			//std::cout << it;

			/*std::cout << "Created end iterator with \ncurrentEntity: " << it.currentEntity
				<< "\narchetypeIndex: " << it.archetypeIndex
				<< "\niteratorVersion: " << it.iteratorVersion;*/

			return it;
		}

		// Not in O(1) time, but still somewhat fast
		Iterator operator[](int idx) {
			Iterator it = begin();

			while (idx > 0) {
				if (idx > storageSizes[it.archetypeIndex]) {
					idx -= storageSizes[it.archetypeIndex] + 1;
					it.archetypeIndex++;
				}
				else {
					it.currentEntity = idx;
					idx = 0;
				}
			}
			return *it;
		}

		template <typename U>
		U* getPointer(ComponentType id, uint32_t entityIdx, uint32_t archetypeIdx) {
			//std::cout << "Component id: " << id << "Entity index: " << entityIdx << "Archetype Index: " << archetypeIdx << "\n";
			return static_cast<U*>((componentStarts.template get<U>()[archetypeIdx]) + entityIdx);
		}

	private:
		EntityManager* em = nullptr;
		std::vector<Archetype*>* archetypes = nullptr;


		//------------------------------------
		// Structs to access into componentStorages
		// smoothly inside the iterator
		//------------------------------------
		

		// Needed to write ComponentStarts (I think)
		//template <typename... _Ts>
		struct ComponentIndices {
			std::array<std::vector<std::size_t>, N> indices;

			/*static auto indices = meta::make_tuple_from_pack<Ts...>([](auto type_tag) -> decltype(auto) {
				std::vector<std::size_t> vec;
				return vec;
				});*/


			// I need to map the type to the position inside the indices tuple
			template<typename U>
			std::vector<std::size_t>& get() {
				constexpr std::size_t idx = meta::indexOfType<U, Ts...>();
				return std::get<idx>(indices);
			}

			template<typename U>
			const std::vector<std::size_t>& get() const {
				constexpr std::size_t idx = meta::indexOfType<U, Ts...>();
				return std::get<idx>(indices);
			}


			// Basically same as in ComponentStarts, somewhat redundant
			void update(const std::vector<Archetype*>* archetypes) {
				if (std::get<0>(indices).size() == (*archetypes).size()) {
					return;
				}

				meta::forEachType<Ts...>([&](auto iConst) {
					constexpr std::size_t I = decltype(iConst)::value;
					using type = std::tuple_element_t<I, pack_t>;

					std::vector<std::size_t>& typeIndices = std::get<I>(indices);
					typeIndices.resize((*archetypes).size());

					for (std::size_t i = 0; i < typeIndices.size(); i++) {
						typeIndices[i] = (*archetypes)[i]->findComponentIndex(ecs::getComponentID<type>());
					}
					});
			}
		};
		ComponentIndices componentIdx;

		struct ComponentStarts {
			std::tuple<std::vector<Ts*>...> starts;
			template<typename U> std::vector<U*>& get() {
				constexpr auto  idx = meta::indexOfType<U, Ts...>();
				return std::get<idx>(starts);
			}

			template<typename U> std::vector<U*>& get() const {
				constexpr auto  idx = meta::indexOfType<U, Ts...>();
				return std::get<idx>(starts);
			}

			// Can be sped up once I know which starts will be constant and which will not
			void update(const std::vector<Archetype*>* archetypes,
				const ComponentIndices& compIdx) {
				meta::forEachType<Ts...>([&](auto iConst) {
					constexpr std::size_t I = decltype(iConst)::value;
					using type = std::tuple_element_t<I, pack_t>;

					std::vector<type*>& typeStarts = std::get<I>(starts);
					typeStarts.resize((*archetypes).size());
					//std::cout << "Typestarts size: " << typeStarts.size() << "\n";

					for (std::size_t i = 0; i < typeStarts.size(); i++) {
						// sanity check
						const auto compIndex = compIdx.template get<type>()[i];
						if (compIndex == static_cast<std::size_t>(-1) || compIndex >= (*archetypes)[i]->storages.size()) {
							throw std::runtime_error("ComponentStarts::update: invalid component index for archetype");
						}

						auto& storage = (*archetypes)[i]->storages[compIdx.template get<type>()[i]];
						using S = std::decay_t<decltype(storage)>;
						static_assert(std::is_same_v<S, ComponentStorage>, "Unexpected storage type!");

						typeStarts[i] = (type*)(*archetypes)[i]->storages[compIdx.template get<type>()[i]].data;
					}
					});
			}
		};
		ComponentStarts componentStarts;

		struct StorageSizes {
			std::vector<std::size_t> sizes;

			std::size_t& operator[](std::size_t i) {
				return sizes[i];
			}

			void update(const std::vector<Archetype*>* archetypes) {
				sizes.resize(archetypes->size());
				for (int i = 0; i < sizes.size(); i++) {
					//std::cout << "Size of sizes " << sizes.size() << "\n";
					sizes[i] = (*archetypes)[i]->storageQueue.highestID;
					//std::cout << "Registered size of " << sizes[i] << "\n";
				}
			}
		}; 
		
		StorageSizes storageSizes;

		std::size_t version = 0;

		std::array<ComponentType, N> componentIDs = { ecs::getComponentID<Ts>()... };

		std::bitset<MAX_COMPONENTS> systemSignature = 0;

		void writeSignature() {
			((systemSignature[ecs::getComponentID<Ts>()] = true), ...);
			//std::cout << "Registered system with signature " << systemSignature << "\n";
		}
	};

}