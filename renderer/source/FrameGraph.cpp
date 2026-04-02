#include "FrameGraph.h"
#include "FrameManager.h"
#include <algorithm>
#include <stdexcept>
#include <list>
#include <vulkan/vulkan_core.h>

struct AccessInfo {
    VkPipelineStageFlags2 stage;
    VkAccessFlags2 access;
    VkImageLayout layout;
    VkImageAspectFlagBits aspect;
};

static std::unordered_map<FrameGraphUsageFlag, AccessInfo> accessTable = {

    {FrameGraphUsageFlag::NoUsage,
    {
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_2_NONE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_ASPECT_COLOR_BIT
    }},

    {FrameGraphUsageFlag::ColorWrite,
    {
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT
    }},

    {FrameGraphUsageFlag::ColorRead,
    {
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT
    }},

    {FrameGraphUsageFlag::DepthWrite,
    {
        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,    // Early tests are for more complicated cases, see https://stackoverflow.com/questions/79462374/why-does-early-fragment-tests-need-to-be-specified-in-shader-if-i-write-to-a-sto
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT
    }},

    {FrameGraphUsageFlag::DepthReadWrite,
    {
        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | 
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT
    }}
};
    

enum class FrameGraphUsageFlagAbbrv {
    None,
    Write,
    Read,
    ReadWrite
};

FrameGraphUsageFlagAbbrv abbreviate(FrameGraphUsageFlag flag) {
    if(flag == FrameGraphUsageFlag::ColorWrite) {
        return FrameGraphUsageFlagAbbrv::Write;
    }
    if(flag == FrameGraphUsageFlag::ColorRead) {
        return FrameGraphUsageFlagAbbrv::Read;
    }
    if(flag == FrameGraphUsageFlag::DepthReadWrite) {
        return FrameGraphUsageFlagAbbrv::ReadWrite;
    }
    throw std::logic_error("Could not abbreviate usage flag!");
};

FrameGraph::FrameGraph(std::vector<std::vector<PassInfo>> passes, std::vector<std::vector<ImageBarrierPrototype>> graphBarriers) : barriers(graphBarriers){
    for(auto& column : passes) {
        graph.emplace_back();
        for(auto& pass : column) {
            graph.back().emplace_back(pass.execute);
        }
    }
}

void FrameGraphBuilder::submitPass(PassInfo info) {
    passes.push_back(info);
    for(size_t i = 0; i < info.attachmentCount; i++) {
        if(maxImageHandle < info.pHandles[i]) {
            maxImageHandle = info.pHandles[i];
        }
    }
}

std::vector<FrameGraphUsageFlag> FrameGraphBuilder::getUsages(PassInfo& pass) {
    std::vector<FrameGraphUsageFlag> usages(maxImageHandle);
    for(int i = 0; i < pass.attachmentCount; i++) {
        usages.emplace_back(pass.pUsages[i]);
    }
    return usages;
}



bool FrameGraphBuilder::tryMerge(std::vector<PassInfo>& dst, PassInfo& src) {
    std::vector<FrameGraphUsageFlag> srcUsages = getUsages(src);
    for(auto& d : dst) {
        std::vector<FrameGraphUsageFlag> dstUsages = getUsages(d);
        for(size_t i = 0; i <= maxImageHandle; i++) {
            if(srcUsages[i] != FrameGraphUsageFlag::NoUsage) {
                if(dstUsages[i] != FrameGraphUsageFlag::NoUsage) {
                    return false;
                }
            }
        }
    }
    dst.emplace_back(src);
    return true;
}



ImageNode getNode(ImageHandle image, FrameGraphUsageFlag flag) {
    ImageNode node;
    node.image = image;
    AccessInfo info = accessTable[flag];
    node.aspect = info.aspect;
    node.stageMask = info.stage;
    node.layout = info.layout;
    return node;
}

std::vector<ImageNode> getNodes(std::vector<PassInfo>& passes) {
    std::vector<ImageNode> nodes;
    nodes.reserve(passes.size() - 1);
    for(auto& pass : passes) {
        for(int i = 0; i < pass.attachmentCount; i++)
            nodes.emplace_back(getNode(pass.pHandles[i], pass.pUsages[i]));
    }

    return nodes;
}

std::vector<ImageBarrierPrototype> FrameGraphBuilder::createBarriers(std::vector<ImageNode>& old, std::vector<ImageNode>& next) {
    assert(old.size() == maxImageHandle);
    std::vector<ImageBarrierPrototype> barriers;
    for(auto& node : next) {
        ImageNode oldNode = old[node.image];
        barriers.emplace_back(oldNode.layout, node.layout, node.image, node.aspect);
        old[node.image] = node;
    }

    return barriers;
}


FrameGraph FrameGraphBuilder::build() {
    std::vector<std::vector<PassInfo>> graphPrototype;
    graphPrototype.reserve(passes.size());
    for(auto& pass : passes) {
        graphPrototype.emplace_back(1, pass);
    }

    // Just try and merge things until nothing merges anymore
    // Pretty sure this method is trash, but idc
    // Just need to construct frame graph once
    bool noMerges = false;
    while(!noMerges) {
        noMerges = true;
        for(size_t i = 0; i < graphPrototype.size() - 1; i++) {
            for(size_t j = 0; j < graphPrototype[i + 1].size(); j++) {
                if(tryMerge(graphPrototype[i], graphPrototype[i+1][j])) {
                    noMerges = false;
                    j--;
                }
                if(graphPrototype[i + 1].empty()) {
                    graphPrototype[i + 1].erase(graphPrototype[i + 1].begin() + i);
                    i--;
                }
            }
        }
    }

    // At this point, graphPrototype should have optimal structure
    std::vector<std::vector<ImageBarrierPrototype>> barriers;
    std::vector<ImageNode> previousUses;
    for(int i = 0; i <= maxImageHandle; i++) {
    previousUses.emplace_back(i,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_PIPELINE_STAGE_2_NONE);
    }
    for(int i = 0; i < graphPrototype.size(); i++) {
        std::vector<ImageNode> nextUses = getNodes(graphPrototype[i]);
        barriers.emplace_back(createBarriers(previousUses, nextUses));
    }

    return FrameGraph(graphPrototype, barriers);
    


    

}
