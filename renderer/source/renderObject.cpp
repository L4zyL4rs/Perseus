#include "renderObject.h"

RenderObject::RenderObject(glm::vec3 pos, glm::vec3 vel, MeshHandle objectHandle, float objectScale)
    : viewPosition(pos), velocity(vel), handle(objectHandle), scale(objectScale) {}

ObjectManager::ObjectManager(DescriptorAllocator* dA, PipelineManager* pM, CommandPool* cP, AssetManager* aM, uint32_t* MFIF) 
    : context(dA->context)
    , swapchain(pM->swapchain)
    , descriptorAllocator(dA)
    , pipelineManager(pM)
    , assetManager(aM)
    , commandPool(cP)
    , MAX_FRAMES_IN_FLIGHT(MFIF)
    , window(context->window) {
    viewCameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
    createMeshGraphicsPipeline();
    createGlobalUniformBuffers();
    createGlobalDescriptorSets();
    //std::srand(std::time({}));
    //std::cout << "Rand max: " << RAND_MAX << "\n";
}

void ObjectManager::createMeshGraphicsPipeline() {
    PipelineBuilder meshTemplate;
    meshTemplate.setDefaults(pipelineManager, PipelineType::Mesh);
    meshTemplate.enableDepthWrite();
    meshTemplate.addVertShader("assets/shaders/meshVert.spv");
    meshTemplate.addGeomShader("assets/shaders/meshGeom.spv");
    meshTemplate.addFragShader("assets/shaders/meshFrag.spv");
    meshTemplate.addDescriptor(descriptorAllocator->layouts.camera);
    meshTemplate.addDescriptor(descriptorAllocator->layouts.meshAndSampler);
    meshTemplate.setVertexFormat(Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
    meshTemplate.enablePushConstants(sizeof(MeshPushConstant));

    meshTemplate.build();
}

const std::vector<DrawItem>& ObjectManager::assembleDrawItems(uint32_t currentImage) {
    drawItems.clear();
    //updateGlobalUniformBuffer(currentImage);
    //updateViewPositions();
    const VkBuffer& vertexBuffer = assetManager->getVertexBuffer();
    const VkBuffer& indexBuffer = assetManager->getIndexBuffer();


    for(auto& object : objects) {
        const MeshResource& mesh = assetManager->getMesh(object.handle);
        std::vector<VkDescriptorSet> descriptors;
        descriptors.push_back(globalDescriptorSets[currentImage]);
        descriptors.push_back(assetManager->getDescriptors(mesh.texture)[currentImage]);

        DrawItem item;
        item.descriptorSets = descriptors;
        item.transform = glm::scale(glm::rotate(glm::translate(glm::translate(glm::mat4(1.0f), object.viewPosition), -window->cam.worldCameraPos), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)), object.scale);
        item.indexBuffer = indexBuffer;
        item.meshBuffer = vertexBuffer;
        item.meshStartIndex = mesh.start;
        item.meshStopIndex = mesh.end;
        item.pipeline = PipelineType::Mesh;
        item.sortKey = 0;

        //std::cout << "View position: \n" << object.viewPosition;
        //std::cout << "Push constant: \n" << item.transform;
        drawItems.push_back(item);

    }
    //std::cout << "Returning " << drawItems.size() << " items from object manager\n";
    return drawItems;
}

void ObjectManager::addObject(std::string ID, glm::vec3 position, glm::vec3 velocity, float objectScale) {
    MeshHandle mesh = assetManager->loadAsset(ID);
    objects.emplace_back(position, velocity, mesh);
}

void ObjectManager::createGlobalUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(GlobalUniformBufferObject);

    globalUniformBuffers.resize(*MAX_FRAMES_IN_FLIGHT);
    globalUniformBuffersMemory.resize(*MAX_FRAMES_IN_FLIGHT);
    globalUniformBuffersMapped.resize(*MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < *MAX_FRAMES_IN_FLIGHT; i++) {
        VulkanHelper::createBuffer(context, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &globalUniformBuffers[i], &globalUniformBuffersMemory[i]);

        vkMapMemory(context->device, globalUniformBuffersMemory[i], 0, bufferSize, 0, &globalUniformBuffersMapped[i]);
    }
    updateGlobalUniformBuffer(0);
}

void ObjectManager::updateGlobalUniformBuffer(uint32_t currentImage) {
    lightPos = Int64vector(1, 1, 1);
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec4 viewLightPos = glm::vec4(0, 0, 0, 0);
    viewLightPos.x = static_cast<float>(lightPos.x - window->cam.worldCameraPos.x);
    viewLightPos.y = static_cast<float>(lightPos.y - window->cam.worldCameraPos.y);
    viewLightPos.z = static_cast<float>(lightPos.z - window->cam.worldCameraPos.z);
    /*std::cout << "CameraFrontX: " << window->cam.cameraFront.x << " ";
    std::cout << "CameraFrontY: " << window->cam.cameraFront.y << " ";
    std::cout << "CameraFrontZ: " << window->cam.cameraFront.z << "\n";*/
    GlobalUniformBufferObject ubo{};
    ubo.view = glm::lookAt(viewCameraPos, window->cam.cameraFront + viewCameraPos, window->cam.cameraUp);
    ubo.proj = glm::perspective(glm::radians(45.0f), swapchain->extent.width / (float)swapchain->extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    //std::cout << "projection: \n" << ubo.proj;
    ubo.light = viewLightPos;
    ubo.lightColor = lightColor;
    memcpy(globalUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void ObjectManager::updateUniformBuffer(uint32_t currentImage) {
    updateGlobalUniformBuffer(currentImage);
}

//void ObjectManager::updateViewPositions() {
//    // Calculating float view positions from int64 simulation positions
//    // Camera for these positions is placed at origin
//    for(auto& object : objects) {
//        object.viewPosition.x = static_cast<float>(object.viewPosition.x - window->cam.worldCameraPos.x);
//        object.viewPosition.y = static_cast<float>(object.viewPosition.y - window->cam.worldCameraPos.y);
//        object.viewPosition.z = static_cast<float>(object.viewPosition.z - window->cam.worldCameraPos.z) ;
//        }
//}

void ObjectManager::createGlobalDescriptorSets() {
    globalDescriptorSets.resize(*MAX_FRAMES_IN_FLIGHT);
    descriptorAllocator->allocate(descriptorAllocator->layouts.camera, globalDescriptorSets.data(), *MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < *MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = globalUniformBuffers[0];
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(GlobalUniformBufferObject);

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = globalDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &uniformBufferInfo;
        vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void ObjectManager::cleanup() {
    for (size_t i = 0; i < *MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(context->device, globalUniformBuffers[i], nullptr);
        vkFreeMemory(context->device, globalUniformBuffersMemory[i], nullptr);
    }
}


// Old function for testing, too many objects to just blindly load everything now
//void ObjectManager::scanModels() {
//    // Find all files in the models directory
//    uniqueObjectCount = 0;
//    for (auto& entry : std::filesystem::directory_iterator{ "models/" }) {
//        std::string filepath = entry.path().stem().string();
//        objectNames.push_back(filepath);
//        uniqueObjectCount += 1;
//        // objects.push_back(std::vector<RenderObject*>());
//    }
//}



//---------------------------------------------
// 
//  PHYSICS CODE DOWN HERE
//  
//  DO NOT REMOVE, THIS WORKS in principle
// 
//---------------------------------------------
//
//
//void ObjectManager::physics(std::atomic<bool>& MAIN_LOOP_RUNNING) {
//    gravity = -10;
//    physicsDelta = 10;
//    auto nextStepTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<long>(physicsDelta));
//    while (MAIN_LOOP_RUNNING) {
//        mutex.lock();
//
//        auto start{ std::chrono::steady_clock::now() };
//        // Main physics calculation take place here
//     
//        boxPhysics();
//        collisionPhysics();
//
//        /*for (auto& object : objects) {
//            std::cout << object.velocity[1] << " ";
//        }
//        std::cout << "\n";*/
//
//        auto end{ std::chrono::steady_clock::now() };
//        std::chrono::duration<float> calculationTime{ end - start };
//        //std::cout << calculationTime;
//        mutex.unlock();
//
//        std::this_thread::sleep_until(nextStepTime);
//        nextStepTime += std::chrono::milliseconds(static_cast<long>(physicsDelta));
//    }
//}
//
//void ObjectManager::boxPhysics() {
//    float boxSize = 2500000;
//    for(auto& object : objects) {
//        object.velocity.z += static_cast<int64_t>(gravity * physicsDelta);
//        object.worldPosition.x += static_cast<int64_t>(object.velocity.x * (physicsDelta / 1000));
//        object.worldPosition.y += static_cast<int64_t>(object.velocity.y * (physicsDelta / 1000));
//        object.worldPosition.z += static_cast<int64_t>(object.velocity.z * (physicsDelta / 1000));
//        if (object.worldPosition.x < -boxSize) {
//            object.worldPosition.x += -boxSize - object.worldPosition.x;
//            object.velocity.x *= -1;
//        }
//        if (object.worldPosition.x > boxSize) {
//            object.worldPosition.x += boxSize - object.worldPosition.x;
//            object.velocity.x *= -1;
//        }
//        if (object.worldPosition.y < -boxSize) {
//            object.worldPosition.y += -boxSize - object.worldPosition.y;
//            object.velocity.y *= -1;
//        }
//        if (object.worldPosition.y > boxSize) {
//            object.worldPosition.y += boxSize - object.worldPosition.y;
//            object.velocity.y *= -1;
//        }
//        if (object.worldPosition.z < -boxSize) {
//            object.worldPosition.z += -boxSize - object.worldPosition.z;
//            object.velocity.z *= -1;
//        }
//        if (object.worldPosition.z > boxSize) {
//            object.worldPosition.z += boxSize - object.worldPosition.z;
//            object.velocity.z *= -1;
//        }
//    }
//}
//
//void ObjectManager::collisionPhysics() {
//
//    struct Endpoint {
//        float x;
//        int objectIndex;
//        bool isStart;
//    };
//
//    // Build endpoints for sweep
//    std::vector<Endpoint> endpoints;
//    for (int i = 0; i < objects.size(); ++i) {
//        RenderObject& obj = objects[i];
//        float minX = static_cast<float>(obj.worldPosition.x - obj.hitboxRadius);
//        float maxX = static_cast<float>(obj.worldPosition.x + obj.hitboxRadius);
//        endpoints.push_back({ minX, i, true });
//        endpoints.push_back({ maxX, i, false });
//    }
//
//    // Sort endpoints
//    std::sort(endpoints.begin(), endpoints.end(), [](const Endpoint& a, const Endpoint& b) {
//        return a.x < b.x || (a.x == b.x && a.isStart > b.isStart); // start before end
//        });
//
//    // Sweep & Prune collision check
//    std::set<int> activeSet;
//    for (const auto& ep : endpoints) {
//        int index = ep.objectIndex;
//        RenderObject& objA = objects[index];
//
//        if (ep.isStart) {
//            for (int otherIndex : activeSet) {
//                RenderObject& objB = objects[otherIndex];
//
//                Int64vector delta = objA.worldPosition - objB.worldPosition;
//                int64_t distSq = delta * delta;
//                int64_t radiusSum = objA.hitboxRadius + objB.hitboxRadius;
//
//                if (distSq <= radiusSum * radiusSum) {
//                    // Handle collision physics
//
//                    Int64vector normal = delta.normalized();
//                    int64_t proj1 = objA.velocity * normal;
//                    int64_t proj2 = objB.velocity * normal;
//
//                    int64_t mass1 = objA.mass;
//                    int64_t mass2 = objB.mass;
//
//                    int64_t newProj1 = (proj1 * (mass1 - mass2) + 2 * mass2 * proj2) / (mass1 + mass2);
//                    int64_t newProj2 = (proj2 * (mass2 - mass1) + 2 * mass1 * proj1) / (mass1 + mass2);
//
//                    // Update velocities along collision normal
//                    objA.velocity = objA.velocity + (newProj1 - proj1) * normal;
//                    objB.velocity = objB.velocity + (newProj2 - proj2) * normal;
//
//                    // Resolve overlap
//                    float dist = std::sqrt(static_cast<float>(distSq));
//                    if (dist < 1e-4f) dist = 1e-4f; // prevent division by zero
//
//                    float overlap = static_cast<float>(radiusSum) - dist;
//                    Int64vector correction = normal * static_cast<int64_t>(overlap * 0.55f); // push both slightly
//
//                    objA.worldPosition = objA.worldPosition + correction;
//                    objB.worldPosition = objB.worldPosition - correction;
//                }
//            }
//            activeSet.insert(index);
//        }
//        else {
//            activeSet.erase(index);
//        }
//    }
//}
//
//std::vector<float> ObjectManager::generateHitboxIntervals() {
//    std::vector<float> hitboxIntervals;
//    for(auto& object : objects) {
//        hitboxIntervals.push_back(object.worldPosition.x - object.hitboxRadius);
//        hitboxIntervals.push_back(object.worldPosition.x + object.hitboxRadius);
//    }
//    return hitboxIntervals;
//}