#include "FrameManager.h"

FrameManager::FrameManager(Swapchain* s) : swapchain(s)
, context(swapchain->context)
, window(context->window)
, pipelineManager(context, swapchain)
, commandPool(context)
, MAX_FRAMES_IN_FLIGHT(1)
, descriptorAllocator(context, MAX_OBJECTS, &MAX_FRAMES_IN_FLIGHT)
, assetManager(context, &commandPool, &descriptorAllocator)
, objectManager(&descriptorAllocator, &pipelineManager, &commandPool, &assetManager, &MAX_FRAMES_IN_FLIGHT)
, fontRenderer(context, &descriptorAllocator, &pipelineManager, &assetManager, &commandPool)
{
	createCommandBuffers();
	createSyncObjects();
	assetManager.createBuffers();
	MAIN_LOOP_RUNNING = true;
	//physicsThread = std::thread(&ObjectManager::physics, &objectManager, std::ref(MAIN_LOOP_RUNNING));
}

void FrameManager::cleanup()
{
	MAIN_LOOP_RUNNING = false;
	physicsThread.join();
	descriptorAllocator.reset();
	pipelineManager.cleanup();
	objectManager.cleanup();
	fontRenderer.cleanup();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(context->device, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(context->device, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(context->device, inFlightFences[i], nullptr);
	}
}

void FrameManager::createCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool.get();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(context->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void FrameManager::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, std::vector<DrawItem>& drawItems)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapchain->renderPass;
	renderPassInfo.framebuffer = swapchain->framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchain->extent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchain->extent.width);
	viewport.height = static_cast<float>(swapchain->extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchain->extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Passing 0 instead of the current Frame in flight is bad
	// Rendering itself is done "correctly" now, but everything gets rendered at origin because physics is not running

	drawDrawItems(drawItems, commandBuffer, currentFrame);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void FrameManager::draw(std::vector<DrawItem>& drawItems)
{
	vkWaitForFences(context->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;    // SPECIFIES WHICH IMAGE IS DRAWN TO AND PRESENTED
	VkResult result = vkAcquireNextImageKHR(context->device, swapchain->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		swapchain->recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(context->device, 1, &inFlightFences[currentFrame]);

	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex, drawItems);

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(context->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkSwapchainKHR swapChains[] = { swapchain->swapChain };

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(context->getPresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window->framebufferResized) {
		window->framebufferResized = false;
		swapchain->recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void FrameManager::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(context->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

void FrameManager::drawDrawItems(const std::vector<DrawItem>& items, VkCommandBuffer commandBuffer, uint32_t currentFrame)
{
	for (auto& item : items) {
		// Just always rebind everything for now like a caveman
		// Driver should probably make this a no-op but idk
		VkDeviceSize offsets[] = { 0 };
		//std::cout << item.meshBuffer
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.pipeline(item.pipeline));
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &item.meshBuffer, offsets);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.getPipelineLayout(item.pipeline), 0, item.descriptorSets.size(), &item.descriptorSets[0], 0, 0);
		MeshPushConstant pushConstant{ item.transform };
		if (pipelineManager.pushConstantSizes[item.pipeline] != 0) {
			vkCmdPushConstants(commandBuffer, pipelineManager.getPipelineLayout(item.pipeline), VK_SHADER_STAGE_VERTEX_BIT, 0, pipelineManager.pushConstantSizes[item.pipeline], &pushConstant);
		}

		if (item.indexBuffer == nullptr) {
			vkCmdDraw(commandBuffer, item.meshStopIndex - item.meshStartIndex + 1, 1, item.meshStartIndex, 0);
		}
		else {
			vkCmdBindIndexBuffer(commandBuffer, item.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			std::cout << "Start index " << item.meshStartIndex << "\n";
			vkCmdDrawIndexed(commandBuffer, item.meshStopIndex - item.meshStartIndex + 1, 1, item.meshStartIndex, 0, 0);
		}
	}
}

