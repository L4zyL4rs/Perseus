#pragma once
#include "DescriptorAllocator.h"
#include "Bitmap.h"
#include "glm_config.h"

// Pipeline, material, depth and mesh get 16 bits
// Together they form a 64 bit number we can sort by
using MeshHandle = uint16_t;
using TextureHandle = uint16_t;
using FontHandle = uint16_t;

struct Vertex {
    glm::vec3 pos{ 0.0f };
    glm::vec3 color{ 1.0f };
    glm::vec2 texCoord{ 0.0f };

    bool operator==(const Vertex& other) const;
    static VkVertexInputBindingDescription getBindingDescription();

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

namespace std {
    template<> struct hash<Vertex> {
		inline size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
    };
};

struct CharacterInfo {
    glm::vec2 uv0{};
    glm::vec2 uv1{};
    float u0{}, v0{}, u1{}, v1{};   // UV rectangle in atlas
    int width{}, height{};     // Glyph size in pixels
    int bearingX{}, bearingY{};
    int advance{};
};

struct Texture {
    std::string name{};
    RenderContext* context{};
    VkImage image{};
    VkDeviceMemory imageMemory{};
    VkImageView imageView{};
    VmaAllocation imageAllocation{};
    VkSampler sampler{};
    uint32_t mipLevels{};

    glm::mat4 materialData{};
    std::vector<VkBuffer> materialDataBuffers{};
    std::vector<VkDeviceMemory> materialDataBuffersMemory{};
    std::vector<void*> materialDataBuffersMapped{};

    std::vector<VkDescriptorSet> descriptors{};
};

// For now just indices into index buffer
// Valid as long as one big vertex buffer is used
// Accessing textures should be done through TextureHandles in the future to share textures across meshes
// Too lazy to do that now, only important when instancing
struct MeshResource {
    uint32_t start{};
    uint32_t end{};
    std::string name{};
    TextureHandle texture = UINT16_MAX;
};

// Struct containing only the relevant rendering information for one character
// Gets fed into vertex shader
struct CharacterCoordinates {
    glm::vec2 textureUv{};
    glm::vec2 screenUv{};
    glm::vec4 color{};

    static VkVertexInputBindingDescription getBindingDescription();

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

struct FontResource {
    std::string path{};
    int fontSize{};
    VkImageView atlas{};
    std::unordered_map<char, CharacterInfo> characters{};
    std::vector<VkDescriptorSet> atlasDescriptorSet{};
    VmaAllocation atlasImageAllocation{};
    VkImage atlasImage{};
    VkDeviceMemory atlasImageMemory{};
};

// Originally meant for transform, obsolete with push constants
// Keep it for material data or whatever
struct LocalUniformBufferObject {
    alignas(16) glm::mat4 model{};
};

class AssetManager {
public:
    std::vector<MeshResource> meshResources{};
    std::vector<Texture> textureResources{};
    std::vector<FontResource> fontResources{};

    AssetManager(RenderContext* c, CommandPool* cP, DescriptorAllocator* dA);

    FontHandle loadFont(std::string& path, size_t fontSize);

    // LOADED MODELS WILL NOT BE RENDERABLE UNTIL createBuffers() IS CALLED
    // Assets should later be some .yaml file that contains all texture, mesh and material data
	MeshHandle loadAsset(std::string& name);

    const FontResource& getFont(FontHandle handle);

    const MeshResource& getMesh(MeshHandle handle);

    const std::vector<VkDescriptorSet>& getDescriptors(TextureHandle handle) const;

    void createBuffers();

    void cleanup();

    const VkBuffer& getIndexBuffer();

    const VkBuffer& getVertexBuffer();

private:
    RenderContext* context{};
    CommandPool* commandPool{};
    DescriptorAllocator* descriptorAllocator{};
    
    std::vector<uint32_t> indices{};
    VkBuffer indexBuffer{};
    VkDeviceMemory indexBufferMemory{};

    std::vector<Vertex> vertices{};
    VkBuffer vertexBuffer{};
    VkDeviceMemory vertexBufferMemory{};
    VkSampler fontSampler{};
    bool buffersCreated = false;

    void loadMesh(MeshHandle handle);
    void createTexture(TextureHandle handle);
    void createTextureImage(TextureHandle handle);
    void createTextureImageView(TextureHandle handle);
    void createTextureSampler(TextureHandle handle);
    void createMaterialData(TextureHandle handle);
    void loadMaterialData(TextureHandle handle);
    void createMaterialBuffers(TextureHandle handle);
    void updateMaterialBuffers(TextureHandle handle, int currentFrame);
    void createTextureDescriptorSets(TextureHandle handle);
    void createIndexBuffer();
    void createVertexBuffer();

    // Objects should clean up their resource as they might be shared, but ignore this for now
    void cleanupObject(MeshHandle handle);
    void cleanupFont(FontHandle handle);
    void createAtlasImage(FontResource& fontResource, Bitmap& bitmap);
    void createAtlasImageView(FontResource& fontResource, Bitmap& bitmap);
    // Loads ASCII glyphs (32 to 126) into a single bitmap
    Bitmap loadFontBitmap(const std::string& fontPath, int fontSize, FontResource& resource, int padding = 2);
    void createAtlasDescriptorSet(FontResource& resource);

    // Sampler used for rendering all fonts
    void createFontSampler();
};
