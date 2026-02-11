#pragma once
#include "FontRenderer.h"
#include <unordered_map>

FontRenderer::FontRenderer(RenderContext* c, DescriptorAllocator* dA, PipelineManager* pM, AssetManager* aM, CommandPool* cP)
    : context(c)
    , descriptorAllocator(dA)
    , pipelineManager(pM)
    , assetManager(aM)
    , commandPool(cP) {
    createTextGraphicsPipeline();
    initStagingBuffer(characterBufferSize);
    createCharacterBuffer();
    createTextElement(fontPathTerminal, 200, "Elden Ring Goonersquad", glm::vec2(-0.5, 0.0));
}

void FontRenderer::createTextGraphicsPipeline() {
    PipelineBuilder textTemplate;
    textTemplate.setDefaults(pipelineManager, PipelineType::Text);
    textTemplate.enableDepthWrite();
    textTemplate.setBlendModeAlpha();
    textTemplate.addVertShader("assets/shaders/textVert.spv");
    textTemplate.addFragShader("assets/shaders/textFrag.spv");
    textTemplate.setVertexFormat(CharacterCoordinates::getBindingDescription(), CharacterCoordinates::getAttributeDescriptions());
    textTemplate.addDescriptor(descriptorAllocator->layouts.fontSampler);

    textTemplate.build();
}

void FontRenderer::cleanup() {
    destroyCharacterBuffer();
    destroyStagingBuffer();
}

void FontRenderer::createCharacterBuffer() {
    VulkanHelper::createBuffer(context, characterBufferSize * 6, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &characterBuffer, &characterBufferMemory);
}

// For now fully recreate drawItems
const std::vector<DrawItem>& FontRenderer::assembleDrawItems(std::unordered_map<FontHandle, std::vector<CharacterCoordinates>>& texts, std::vector<CharacterCoordinates>& textBuffer) {
    drawItems.clear();

    //Assign each font its own drawItem
    for (auto& font : fontHandles) {
        if (texts[font].size() == 0) {
            continue;
        }
        const FontResource& resource = assetManager->getFont(font);
        DrawItem drawItem;
        drawItem.descriptorSets = resource.atlasDescriptorSet;
        drawItem.meshStartIndex = textBuffer.size();
        textBuffer.append_range(texts[font]);
        drawItem.meshStopIndex = textBuffer.size() - 1;
        drawItem.meshBuffer = characterBuffer;
        drawItem.indexBuffer = nullptr;
        drawItem.pipeline = PipelineType::Text;
        drawItem.sortKey = 0;
        
        drawItems.push_back(drawItem);
        std::cout << "Text Buffer has size " << textBuffer.size() << "\n";
    }

    // Beware that characterBufferSize might not be big enough!
    memcpy(pStagingBuffer, textBuffer.data(), textBuffer.size() * sizeof(CharacterCoordinates));
    VulkanHelper::copyBuffer(context, commandPool->get(), stagingBuffer, characterBuffer, characterBufferSize);

    std::cout << "Returning " << drawItems.size() << " items from font manager\n";
    return drawItems;
}


void FontRenderer::destroyCharacterBuffer() {
    vkDestroyBuffer(context->device, characterBuffer, nullptr);
    vkFreeMemory(context->device, characterBufferMemory, nullptr);
}

// Generates the corresponding CharacterCoordinates structs belonging to the text input, and writes them to the TextElement
// Build the vertices like this:
//
//       v0-----v1
//       |     / |
//       |   /   |
//       | /     |
//       v2-----v3
//
void FontRenderer::createCharacterCoordinates(std::string text, TextElement* pTextElement) {
    std::vector<CharacterCoordinates> coordinates;
    const FontResource& font = assetManager->getFont(pTextElement->handle);
    coordinates.reserve(text.length() * 6);

    glm::vec2 pen = pTextElement->screenPosUv;
    glm::vec2 rect = glm::vec2(context->window->WINDOWWIDTH, context->window->WINDOWHEIGHT);

    for (char ch : text) {
        //for(auto& letter : pTextElement->fontBundle->characters)
        std::cout << "\nLoading character: " << ch;
        const CharacterInfo& character = font.characters.at(ch);

        glm::vec2 yBearing = glm::vec2(0.0, - character.bearingY) / rect;

        // Screen space positions
        glm::vec2 topLeft = pen + yBearing;
        glm::vec2 topRight = pen + glm::vec2(character.width, 0.0) / rect + yBearing;
        glm::vec2 bottomLeft = pen + glm::vec2(0.0, character.height) / rect + yBearing;
        glm::vec2 bottomRight = pen + glm::vec2(character.width, character.height) / rect + yBearing;

        std::cout << "\nPen TopLeft: " << topLeft.x << "\nPen BottomRight: " << bottomRight.x;

        // Texture UVs
        glm::vec2 uv0 = character.uv0;
        glm::vec2 uv1 = glm::vec2(character.uv1.x, character.uv0.y);
        glm::vec2 uv2 = glm::vec2(character.uv0.x, character.uv1.y);;
        glm::vec2 uv3 = character.uv1;
        glm::vec4 color = glm::vec4(1.0); // Make this more elaborate some day

        std::cout << "\nTexture x TopLeft: " << uv0.x << "\nTexture BottomRight: " << uv3.x;

        // First triangle
        //coordinates.push_back({ uv0,    topLeft,     color });
        //coordinates.push_back({ uv1,    topRight,    color });
        //coordinates.push_back({ uv2,    bottomLeft,  color });

        // Second triangle
        //coordinates.push_back({ uv1,    topRight,    color });
        //coordinates.push_back({ uv2,    bottomLeft,  color });
        //coordinates.push_back({ uv3,    bottomRight, color });

        // Triangle 1
        coordinates.push_back({ uv0, topLeft,     color });
        coordinates.push_back({ uv2, bottomLeft,  color });
        coordinates.push_back({ uv3, bottomRight, color });

        // Triangle 2
        coordinates.push_back({ uv0, topLeft,     color });
        coordinates.push_back({ uv3, bottomRight, color });
        coordinates.push_back({ uv1, topRight,    color });

        pen.x += float(character.advance) / context->window->WINDOWWIDTH;
    }

    pTextElement->text.append_range(coordinates);
}

void FontRenderer::initStagingBuffer(int size) {
    VulkanHelper::createBuffer(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
    vkMapMemory(context->device, stagingBufferMemory, 0, size, 0, &pStagingBuffer);
}

void FontRenderer::destroyStagingBuffer() {
    vkUnmapMemory(context->device, stagingBufferMemory);
    vkDestroyBuffer(context->device, stagingBuffer, nullptr);
    vkFreeMemory(context->device, stagingBufferMemory, nullptr);
}

// This function should probably return some handle to the textElement, so it can actually be accessed again
TextElement FontRenderer::createTextElement(std::string fontPath, int fontSize, std::string text, glm::vec2 pos) {
    // I think I found out how cool pointers are when I wrote this
    // Why
    TextElement textElement;
    textElement.screenPosUv = pos;
    FontHandle handle = assetManager->loadFont(fontPath, fontSize);
    std::cout << "\nloaded handle " << handle << "\n";

    textElement.handle = handle;
    createCharacterCoordinates(text, &textElement);
    std::cout << "text size " << textElement.text.size() << "\n";
    fontHandles.push_back(handle);
    return textElement;
    //assembleDrawItems();
}

//void FontRenderer::draw(VkCommandBuffer commandBuffer, uint32_t currentFrame) {
//    // IDEA:
//    // Have a very generous characterBuffer for all FontBundles
//    // Index into characterBuffer to access the respective font
//    // Maybe there can be some flag in CharacterCoordinates to still store "empty" elements that are not drawn in shader?
//    VkBuffer characterBuffers[] = { characterBuffer };
//    VkDeviceSize offsets[] = {0};
//
//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager->pipeline(PipelineType::Text));
//    vkCmdBindVertexBuffers(commandBuffer,
//        0,
//        1,
//        characterBuffers,
//        offsets);
//
//    for (auto& handle : fontHandles) {
//        FontResource resource = assetManager->getFont(handle);
//        vkCmdBindDescriptorSets(commandBuffer, 
//            VK_PIPELINE_BIND_POINT_GRAPHICS, 
//            pipelineManager->getPipelineLayout(PipelineType::Text), 
//            0, 
//            1,
//            &resource.atlasDescriptorSet[currentFrame],
//            0,
//            0);
//
//        // Not using indexing here is stupid but I can't be bothered rn
//        vkCmdDraw(commandBuffer, resource.lastIndex - font->firstIndex + 1, 1, font->firstIndex, 0);
//    }
//}