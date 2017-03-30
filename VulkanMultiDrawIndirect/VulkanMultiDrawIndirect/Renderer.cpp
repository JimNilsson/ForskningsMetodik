#include "Renderer.h"
#undef min
#undef max
#include <array>
#include <algorithm>
#include <fstream>
#include <Parsers.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;

Renderer::Renderer(HWND hwnd, uint32_t width, uint32_t height):_width(width), _height(height)
{

	/************Create Instance*************/
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	const auto vkAppInfo = &VulkanHelpers::MakeApplicationInfo(
		"Vulkan MDI",
		VK_MAKE_VERSION(1, 0, 0),
		"Frengine",
		VK_MAKE_VERSION(2, 0, 0)
	);
	std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_win32_surface", VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	const auto vkInstCreateInfo = VulkanHelpers::MakeInstanceCreateInfo(
		0,
		vkAppInfo,
		validationLayers.size(),
		validationLayers.data(),
		nullptr,
		extensions.size(),
		extensions.data()
	);
	VulkanHelpers::CreateInstance(&vkInstCreateInfo, &_instance);
	
	/*Create debug callback*/
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	createInfo.pfnCallback = VulkanHelpers::debugCallback;

	if (VulkanHelpers::CreateDebugReportCallbackEXT(_instance, &createInfo, nullptr, &_debugCallback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
	
	/***********Enumerate physical devices*************/
	_devices = VulkanHelpers::EnumeratePhysicalDevices(_instance);
	if (_devices.size() == 0)
		throw std::runtime_error("No devices, pesant");


	/***************Make sure the device has a queue that can handle rendering*****************/
	auto queueFamInfo = VulkanHelpers::EnumeratePhysicalDeviceQueueFamilyProperties(_instance);
	size_t queueIndex = -1;
	
	for (uint32_t i = 0; i <  queueFamInfo[0].size(); i++)
	{
		if (queueFamInfo[0][i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queueIndex = i;
			break;
		}
	}
	if (queueIndex == -1)
		throw std::runtime_error("No queue can render");



	/*************Create the device**************/
	float queuePriority = 1.0f;
	auto queueInfo = VulkanHelpers::MakeDeviceQueueCreateInfo(queueIndex, 1, &queuePriority);
	vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };
	auto lInfo = VulkanHelpers::MakeDeviceCreateInfo(1, &queueInfo, 0, nullptr, nullptr, nullptr, deviceExtensions.size(), deviceExtensions.data());
	VulkanHelpers::CreateLogicDevice(_devices[0], &lInfo, &_device);

	// Get the queue
	vkGetDeviceQueue(_device, queueIndex, 0, &_queue);

	// Create command pool
	auto cmdPoolInfo = VulkanHelpers::MakeCommandPoolCreateInfo(queueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VulkanHelpers::CreateCommandPool(_device, &cmdPoolInfo, &_cmdPool);

	// Allocate cmd buffer
	VulkanHelpers::AllocateCommandBuffers(_device, &_cmdBuffer, _cmdPool);
	VulkanHelpers::AllocateCommandBuffers(_device, &_blitCmdBuffer, _cmdPool);


	_CreateSurface(hwnd);
	_CreateSwapChain();
	_CreateSemaphores();
	_CreateOffscreenImage();
	_CreateOffscreenImageView();
	_CreateDepthBufferImage();
	_CreateDepthBufferImageView();
	_CreateRenderPass();
	_CreateFramebuffer();



	auto prop = VulkanHelpers::GetPhysicalDeviceProperties(_devices[0]);
	
	_gpuTimer = new GPUTimer(_device, 1, prop.limits.timestampPeriod);

	_vertexBufferHandler = new VertexBufferHandler(_devices[0], _device, _queue, _cmdBuffer);

	struct pos
	{
		float e[3];
	};
	pos data[100];
	_vertexBufferHandler->CreateBuffer(data, 100, VertexType::Position);

	_CreateShaders();

	_CreateVPUniformBuffer();



	_CreateDescriptorStuff();
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(_device);


	vkDestroyDescriptorSetLayout(_device, _descLayout, nullptr);
	vkDestroyDescriptorPool(_device, _descPool, nullptr);
	delete _vertexBufferHandler;
	delete _gpuTimer;
	vkDestroyShaderModule(_device, _vertexShader, nullptr);
	vkDestroyShaderModule(_device, _fragmentShader, nullptr);
	vkDestroyFramebuffer(_device, _framebuffer, nullptr);
	vkDestroyRenderPass(_device, _renderPass, nullptr);
	vkDestroyImageView(_device, _depthBufferImageView, nullptr);
	vkDestroyImage(_device, _depthBufferImage, nullptr);
	vkFreeMemory(_device, _depthBufferImageMemory, nullptr);
	vkDestroyImageView(_device, _offscreenImageView, nullptr);
	for (auto& texture : _textures)
	{
		vkDestroyImageView(_device, texture._imageView, nullptr);
		vkDestroyImage(_device, texture._image, nullptr);
		vkFreeMemory(_device, texture._memory, nullptr);
	}
	vkDestroyBuffer(_device, _VPUniformBuffer, nullptr);
	vkFreeMemory(_device, _VPUniformBufferMemory, nullptr);
	vkDestroyBuffer(_device, _VPUniformBufferStaging, nullptr);
	vkFreeMemory(_device, _VPUniformBufferMemoryStaging, nullptr);

	vkFreeMemory(_device, _offscreenImageMemory, nullptr);
	vkDestroyImage(_device, _offscreenImage, nullptr);
	vkDestroyCommandPool(_device, _cmdPool, nullptr);
	vkDestroySemaphore(_device, _swapchainBlitComplete, nullptr);
	vkDestroySemaphore(_device, _imageAvailable, nullptr);
	for (auto view : _swapchainImageViews)
	{
		vkDestroyImageView(_device, view, nullptr);
	}
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);
	vkDestroyDevice(_device, nullptr);
	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	VulkanHelpers::DestroyDebugReportCallbackEXT(_instance, _debugCallback, nullptr);
	vkDestroyInstance(_instance, nullptr);
}

void Renderer::Render(void)
{
	vkQueueWaitIdle(_queue);

	_strategy = _nextStrategy;

	//printf("GPU Time: %f\n", _gpuTimer->GetTime(0));

	// Begin rendering stuff while we potentially wait for swapchain image
	_RenderSceneTraditional();

	// While the scene is rendering we can get the swapchain image and begin
	// transitioning it. When it's time to blit we must synchronize to make
	// sure that the image is finished for us to read.
	_BlitSwapchain();
}

Renderer::MeshHandle Renderer::CreateMesh(const std::string & file)
{
	ArfData::Data data;
	ArfData::DataPointers dataPointers;
	if (ParseObj(file.c_str(), &data, &dataPointers) != 0)
	{
		throw runtime_error("Failed to load mesh from file");
	}

	uint32_t positionOffset = _vertexBufferHandler->CreateBuffer(dataPointers.positions, data.NumPos, VertexType::Position);
	uint32_t texcoordOffset = _vertexBufferHandler->CreateBuffer(dataPointers.texCoords, data.NumTex, VertexType::TexCoord);
	uint32_t normalOffset = _vertexBufferHandler->CreateBuffer(dataPointers.normals, data.NumNorm, VertexType::Normal);

	delete[] dataPointers.buffer;
	dataPointers.buffer = nullptr;

	uint32_t meshIndex = _meshes.size();
	_meshes.push_back({ positionOffset, texcoordOffset, normalOffset, data });

	return MeshHandle(meshIndex);
}

uint32_t Renderer::CreateTexture(const char * path)
{
	auto find = _StringToTextureHandle.find(std::string(path));
	if (find != _StringToTextureHandle.end())
		return find->second;

	int imageWidth, imageHeight, imageChannels;
	stbi_uc* imagePixels = stbi_load(path, &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);
	if (!imagePixels)
		throw std::runtime_error(std::string("Could not load image: ").append(path));

	VkDeviceSize imageSize = imageWidth * imageHeight * 4;

	VkImage stagingImage;
	VkDeviceMemory stagingMemory;

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = imageWidth;
	imageCreateInfo.extent.height = imageHeight;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	VulkanHelpers::CreateImage(_device, &imageCreateInfo, &stagingImage, nullptr); //Throws if failed

	VkMemoryRequirements memoryRequirement;
	vkGetImageMemoryRequirements(_device, stagingImage, &memoryRequirement);

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(_devices[0], &memoryProperties); 

	VkMemoryPropertyFlags desiredProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	int32_t memoryTypeIndex = -1;
	for (int32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		/*From the documentation:
		memoryTypeBits is a bitmask and contains one bit set for every supported memory type for the resource.
		Bit i is set if and only if the memory type i in the VkPhysicalDeviceMemoryProperties structure for the physical device is supported for the resource.*/
		if ((memoryRequirement.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & desiredProperties) == desiredProperties)
		{
			memoryTypeIndex = i;
			break;
		}
	}

	if (memoryTypeIndex < 0)
		throw std::runtime_error("Failed to find compatible memory type");

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirement.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(_device, &memoryAllocateInfo, nullptr, &stagingMemory) != VK_SUCCESS)
		throw std::runtime_error(std::string("Could not allocate memory for staging image: ").append(path));

	if(vkBindImageMemory(_device, stagingImage, stagingMemory, 0) != VK_SUCCESS)
		throw std::runtime_error(std::string("Could not bind memory to staging image: ").append(path));

	void* data;
	vkMapMemory(_device, stagingMemory, 0, imageSize, 0, &data);

	VkImageSubresource imageSubresource = {};
	imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresource.mipLevel = 0;
	imageSubresource.arrayLayer = 0;

	VkSubresourceLayout subresourceLayout;
	vkGetImageSubresourceLayout(_device, stagingImage, &imageSubresource, &subresourceLayout);

	//If there's no padding issues, just fill it
	if (subresourceLayout.rowPitch == imageWidth * 4)
	{
		memcpy(data, imagePixels, imageSize);
	}
	else
	{
		//Deal with padding
		uint8_t* bytes = reinterpret_cast<uint8_t*>(data);
		for (int row = 0; row < imageHeight; row++)
		{
			memcpy(&bytes[row * subresourceLayout.rowPitch], &imagePixels[row * imageWidth * 4], imageWidth * 4);
		}
	}

	vkUnmapMemory(_device, stagingMemory);
	stbi_image_free(imagePixels);

	Texture2D texture;
	VulkanHelpers::CreateImage2D(_device, &(texture._image), imageWidth, imageHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	VulkanHelpers::AllocateImageMemory(_device, _devices[0], texture._image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &(texture._memory));
	vkBindImageMemory(_device, texture._image, texture._memory, 0);

	VkCommandBufferAllocateInfo cmdBufAllInf = {};
	cmdBufAllInf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllInf.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllInf.commandPool = _cmdPool;
	cmdBufAllInf.commandBufferCount = 1;
	VkCommandBuffer oneTimeBuffer;
	vkAllocateCommandBuffers(_device, &cmdBufAllInf, &oneTimeBuffer);

	
	VulkanHelpers::BeginCommandBuffer(oneTimeBuffer,VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VulkanHelpers::TransitionImageLayout(_device, stagingImage, oneTimeBuffer, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	VulkanHelpers::TransitionImageLayout(_device, texture._image, oneTimeBuffer, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkImageSubresourceLayers srl = {};
	srl.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	srl.baseArrayLayer = 0;
	srl.mipLevel = 0;
	srl.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = srl;
	region.dstSubresource = srl;
	region.srcOffset = { 0,0,0 };
	region.dstOffset = { 0,0,0 };
	region.extent.width = imageWidth;
	region.extent.height = imageHeight;
	region.extent.depth = 1; 

	vkCmdCopyImage(oneTimeBuffer, stagingImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	VulkanHelpers::TransitionImageLayout(_device, texture._image, oneTimeBuffer, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VulkanHelpers::EndCommandBuffer(oneTimeBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &oneTimeBuffer;
	vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_queue);
	vkFreeCommandBuffers(_device, _cmdPool, 1, &oneTimeBuffer);


	vkDeviceWaitIdle(_device);
	vkDestroyImage(_device, stagingImage, nullptr);
	vkFreeMemory(_device, stagingMemory, nullptr);

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = texture._image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.levelCount = 1;

	vkCreateImageView(_device, &viewInfo, nullptr, &(texture._imageView));
	_StringToTextureHandle[std::string(path)] = _textures.size();
	_textures.push_back(texture);
	return _textures.size() - 1;
}

void Renderer::UseStrategy(RenderStrategy strategy)
{
	_nextStrategy = strategy;
}

// Render the scene in a traditional manner, i.e. rerecord the draw calls to
// work with a dynamic scene.
void Renderer::_RenderSceneTraditional(void)
{
	VkCommandBufferBeginInfo commandBufBeginInfo = {};
	commandBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufBeginInfo.pNext = nullptr;
	commandBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufBeginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(_cmdBuffer, &commandBufBeginInfo);

	_gpuTimer->Start(_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);

	// Do the actual rendering

	array<VkClearValue, 2> clearValues = {};
	clearValues[0] = { 0.2f, 0.4f, 0.6f, 1.0f };
	clearValues[1].depthStencil = { 0.0f, 0 };

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.renderPass = _renderPass;
	beginInfo.framebuffer = _framebuffer;
	beginInfo.renderArea = { 0, 0, _swapchainExtent.width, _swapchainExtent.height };
	beginInfo.clearValueCount = clearValues.size();
	beginInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(_cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdEndRenderPass(_cmdBuffer);

	// TODO: As of now there is no synchronization point between rendering to
	// the offscreen buffer and using that image as blit source later. At the
	// place of this comment we could probably issue an event that is waited on
	// in the blit buffer before blitting to make sure rendering is complete.
	// Don't forget to reset the event when we have waited on it.

	_gpuTimer->End(_cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0);

	vkEndCommandBuffer(_cmdBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_cmdBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
}

// Blits the content of the offscreen buffer to the swapchain image before
// presenting it. The offscreen buffer is assumed to be in the transfer src
// layout.
void Renderer::_BlitSwapchain(void)
{
	uint32_t imageIdx;
	VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _imageAvailable, VK_NULL_HANDLE, &imageIdx);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Swapchain image retrieval not successful");
	}

	VkCommandBufferBeginInfo commandBufBeginInfo = {};
	commandBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufBeginInfo.pNext = nullptr;
	commandBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufBeginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(_blitCmdBuffer, &commandBufBeginInfo);

	// Transition swapchain image to transfer dst
	VkImageMemoryBarrier swapchainImageBarrier = {};
	swapchainImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	swapchainImageBarrier.pNext = nullptr;
	swapchainImageBarrier.srcAccessMask = 0;
	swapchainImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	swapchainImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	swapchainImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapchainImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapchainImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	swapchainImageBarrier.image = _swapchainImages[imageIdx];
	swapchainImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	swapchainImageBarrier.subresourceRange.baseMipLevel = 0;
	swapchainImageBarrier.subresourceRange.levelCount = 1;
	swapchainImageBarrier.subresourceRange.baseArrayLayer = 0;
	swapchainImageBarrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(
		_blitCmdBuffer,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &swapchainImageBarrier);

	// After render pass, the offscreen buffer is in transfer src layout with
	// subpass dependencies set. Now we can blit to swapchain image before
	// presenting.
	// TODO: Just remember to synchronize here
	VkImageBlit blitRegion = {};
	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.mipLevel = 0;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcOffsets[0] = { 0, 0, 0 };
	blitRegion.srcOffsets[1] = { (int)_swapchainExtent.width, (int)_swapchainExtent.height, 1 };
	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.mipLevel = 0;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstOffsets[0] = { 0, 0, 0 };
	blitRegion.dstOffsets[1] = { (int)_swapchainExtent.width, (int)_swapchainExtent.height, 1 };
	vkCmdBlitImage(_blitCmdBuffer, _offscreenImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _swapchainImages[imageIdx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_LINEAR);

	// When blit is done we transition swapchain image back to present
	swapchainImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	swapchainImageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	swapchainImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	swapchainImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	vkCmdPipelineBarrier(
		_blitCmdBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &swapchainImageBarrier);

	vkEndCommandBuffer(_blitCmdBuffer);

	// Submit the blit...

	VkPipelineStageFlags waitDst = VK_PIPELINE_STAGE_TRANSFER_BIT;

	VkSubmitInfo blitSubmitInfo = {};
	blitSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	blitSubmitInfo.pNext = nullptr;
	blitSubmitInfo.waitSemaphoreCount = 1;
	blitSubmitInfo.pWaitSemaphores = &_imageAvailable;
	blitSubmitInfo.pWaitDstStageMask = &waitDst;
	blitSubmitInfo.commandBufferCount = 1;
	blitSubmitInfo.pCommandBuffers = &_blitCmdBuffer;
	blitSubmitInfo.signalSemaphoreCount = 1;
	blitSubmitInfo.pSignalSemaphores = &_swapchainBlitComplete;
	vkQueueSubmit(_queue, 1, &blitSubmitInfo, VK_NULL_HANDLE);

	// ...and present when it's done.

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_swapchainBlitComplete;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.pImageIndices = &imageIdx;
	presentInfo.pResults = nullptr;
	vkQueuePresentKHR(_queue, &presentInfo);
}

const void Renderer::_CreateSurface(HWND hwnd)
{
	/**************** Set up window surface *******************/
	TCHAR cname[256];
	GetClassName(hwnd, cname, 256);
	WNDCLASS wc;
	GetClassInfo(GetModuleHandle(NULL), cname, &wc);

	VkWin32SurfaceCreateInfoKHR wndCreateInfo;
	wndCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	wndCreateInfo.hwnd = hwnd;
	wndCreateInfo.hinstance = wc.hInstance;
	wndCreateInfo.flags = 0;
	wndCreateInfo.pNext = nullptr;
	
	if (vkCreateWin32SurfaceKHR(_instance, &wndCreateInfo, nullptr, &_surface) != VK_SUCCESS)
		throw std::runtime_error("Window surface creation failed.");

	// For validation purposes
	VkBool32 surfaceSupported;
	vkGetPhysicalDeviceSurfaceSupportKHR(_devices[0], 0, _surface, &surfaceSupported);
	if (surfaceSupported == VK_FALSE)
	{
		throw std::runtime_error("Surface is not supported for the physical device!");
	}

}

const void Renderer::_CreateSwapChain()
{
	/************** Set up swap chain ****************/
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> supportedFormats;
	std::vector<VkPresentModeKHR> supportedPresentModes;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_devices[0], _surface, &capabilities);


	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_devices[0], _surface, &formatCount, nullptr);
	supportedFormats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(_devices[0], _surface, &formatCount, supportedFormats.data());

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(_devices[0], _surface, &presentModeCount, nullptr);
	supportedPresentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(_devices[0], _surface, &presentModeCount, supportedPresentModes.data());

	VkSurfaceFormatKHR bestFormat;
	if (supportedFormats.size() == 1 && supportedFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		bestFormat = supportedFormats[0];
	}
	else
	{
		//Settle for first format unless something better comes along
		bestFormat = supportedFormats[0];
		for (const auto& i : supportedFormats)
		{
			if (i.format == VK_FORMAT_B8G8R8A8_UNORM && i.format == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				bestFormat = i;
				break;
			}
		}
	}
	_swapchainFormat = bestFormat.format;
	//Unless something better comes along, use the standard mode
	VkPresentModeKHR bestPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& i : supportedPresentModes)
	{
		if (i == VK_PRESENT_MODE_MAILBOX_KHR)
			bestPresentMode = i;
	}

	VkExtent2D bestExtent = { _width, _height };
	bestExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, _width));
	bestExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, _height));
	_swapchainExtent = bestExtent;

	uint32_t imageCount = std::min(capabilities.minImageCount + 1, capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR swapCreateInfo;
	ZeroMemory(&swapCreateInfo, sizeof(swapCreateInfo));
	swapCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapCreateInfo.surface = _surface;
	swapCreateInfo.minImageCount = imageCount;
	swapCreateInfo.imageFormat = bestFormat.format;
	swapCreateInfo.imageColorSpace = bestFormat.colorSpace;
	swapCreateInfo.imageExtent = bestExtent;
	swapCreateInfo.imageArrayLayers = 1;
	swapCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapCreateInfo.preTransform = capabilities.currentTransform;
	swapCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapCreateInfo.presentMode = bestPresentMode;
	swapCreateInfo.clipped = VK_TRUE;
	swapCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	//Assume graphics family is the same as present family
	swapCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapCreateInfo.queueFamilyIndexCount = 0;
	swapCreateInfo.pQueueFamilyIndices = nullptr;

	if (vkCreateSwapchainKHR(_device, &swapCreateInfo, nullptr, &_swapchain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create swapchain");

	uint32_t swapchainImageCount = 0;
	if (vkGetSwapchainImagesKHR(_device, _swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to get swapchain image count!");
	}
	_swapchainImages.resize(swapchainImageCount);
	if (vkGetSwapchainImagesKHR(_device, _swapchain, &swapchainImageCount, _swapchainImages.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to get swapchain images!");
	}

	_swapchainImageViews.resize(_swapchainImages.size());
	for (uint32_t i = 0; i < _swapchainImages.size(); ++i)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.pNext = nullptr;
		viewInfo.flags = 0;
		viewInfo.image = _swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = _swapchainFormat;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(_device, &viewInfo, nullptr, &_swapchainImageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create swapchain image view!");
	}
}

void Renderer::_CreateSemaphores(void)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	VkResult result = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailable);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to create image available semaphore!");
	}

	result = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_swapchainBlitComplete);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to create render complete semaphore!");
	}
}

bool Renderer::_AllocateMemory(VkMemoryPropertyFlagBits desiredProps, const VkMemoryRequirements& memReq, VkDeviceMemory& memory)
{
	uint32_t memTypeBits = memReq.memoryTypeBits;
	VkDeviceSize memSize = memReq.size;

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(_devices[0], &memProps);
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		// Current memory type (i) suitable and the memory has desired properties.
		if ((memTypeBits & (1 << i)) && ((memProps.memoryTypes[i].propertyFlags & desiredProps) == desiredProps))
		{
			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.pNext = nullptr;
			allocInfo.allocationSize = memSize;
			allocInfo.memoryTypeIndex = i;

			VkResult result = vkAllocateMemory(_device, &allocInfo, nullptr, &memory);
			if (result == VK_SUCCESS)
			{
				return true;
			}
		}
	}

	return false;
}

void Renderer::_CreateOffscreenImage(void)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = nullptr;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.extent = { _swapchainExtent.width, _swapchainExtent.height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult result = vkCreateImage(_device, &imageInfo, nullptr, &_offscreenImage);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create offscreen image!");
	}

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(_device, _offscreenImage, &memReq);

	if (!_AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memReq, _offscreenImageMemory))
	{
		throw runtime_error("Failed to allocate memory for offscreen image!");
	}

	result = vkBindImageMemory(_device, _offscreenImage, _offscreenImageMemory, 0);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to bind offscreen image to memory!");
	}
}

void Renderer::_CreateOffscreenImageView(void)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = nullptr;
	viewInfo.flags = 0;
	viewInfo.image = _offscreenImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY , VK_COMPONENT_SWIZZLE_IDENTITY };
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkResult result = vkCreateImageView(_device, &viewInfo, nullptr, &_offscreenImageView);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to create offscreen image view!");
	}
}

void Renderer::_CreateDepthBufferImage(void)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = nullptr;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
	imageInfo.extent = { _swapchainExtent.width, _swapchainExtent.height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult result = vkCreateImage(_device, &imageInfo, nullptr, &_depthBufferImage);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create depth buffer!");
	}

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(_device, _depthBufferImage, &memReq);

	if (!_AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memReq, _depthBufferImageMemory))
	{
		throw runtime_error("Failed to allocate memory for depth buffer!");
	}

	result = vkBindImageMemory(_device, _depthBufferImage, _depthBufferImageMemory, 0);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to bind depth buffer to memory!");
	}
}

void Renderer::_CreateDepthBufferImageView(void)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = nullptr;
	viewInfo.flags = 0;
	viewInfo.image = _depthBufferImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
	viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY , VK_COMPONENT_SWIZZLE_IDENTITY };
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkResult result = vkCreateImageView(_device, &viewInfo, nullptr, &_depthBufferImageView);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to create depth buffer image view!");
	}
}

void Renderer::_CreateRenderPass(void)
{
	array<VkAttachmentDescription, 2> attachments = {};
	attachments[0].flags = 0;
	attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	attachments[1].flags = 0;
	attachments[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depthRef;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	array<VkSubpassDependency, 2> subpassDependencies = {};
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	VkRenderPassCreateInfo passInfo = {};
	passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	passInfo.pNext = nullptr;
	passInfo.flags = 0;
	passInfo.attachmentCount = attachments.size();
	passInfo.pAttachments = attachments.data();
	passInfo.subpassCount = 1;
	passInfo.pSubpasses = &subpass;
	passInfo.dependencyCount = subpassDependencies.size();
	passInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(_device, &passInfo, nullptr, &_renderPass);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass!");
	}
}

void Renderer::_CreateFramebuffer(void)
{
	array<VkImageView, 2> attachments = { _offscreenImageView, _depthBufferImageView };

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = nullptr;
	framebufferInfo.flags = 0;
	framebufferInfo.renderPass = _renderPass;
	framebufferInfo.attachmentCount = attachments.size();
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = _swapchainExtent.width;
	framebufferInfo.height = _swapchainExtent.height;
	framebufferInfo.layers = 1;

	VkResult result = vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffer);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to create framebuffer!");
	}
}

void Renderer::_CreateShaders(void)
{
	_CreateShader("../Assets/Shaders/vertex.spv", _vertexShader);
	_CreateShader("../Assets/Shaders/fragment.spv", _fragmentShader);
}

void Renderer::_CreateShader(const char * shaderCode, VkShaderModule & shader)
{
	// Open the file and read to string
	ifstream file(shaderCode, ios::binary | ios::ate);
	if (!file)
	{
		throw runtime_error("Failed to open shader file!");
	}

	streampos codeSize = file.tellg();
	char* spirv = new char[codeSize];
	file.seekg(0, ios::beg);
	file.read(spirv, codeSize);
	file.close();

	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.pNext = nullptr;
	shaderInfo.flags = 0;
	shaderInfo.codeSize = codeSize;
	shaderInfo.pCode = (uint32_t*)spirv;

	VkResult result = vkCreateShaderModule(_device, &shaderInfo, nullptr, &shader);
	if (result != VK_SUCCESS)
	{
		delete[] spirv;
		throw runtime_error("Failed to create shader!");
	}

	delete[] spirv;
	spirv = nullptr;
}

void Renderer::_CreateDescriptorStuff()
{
	/* Create the descriptor pool*/
	std::vector<VkDescriptorPoolSize> _poolSizes = {
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
		{VK_DESCRIPTOR_TYPE_SAMPLER, 1}
	};


	VulkanHelpers::CreateDescriptorPool(_device, &_descPool, 0, 10, 3, _poolSizes.data());



	/* Specify the bindings */
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for (uint32_t i = 0; i < 4; i++)
	{
		bindings.push_back({
			(uint32_t)bindings.size(),
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			VK_SHADER_STAGE_VERTEX_BIT,
			nullptr
		});
	}

	bindings.push_back({
		(uint32_t)bindings.size(),
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	});
	bindings.push_back({
		(uint32_t)bindings.size(),
		VK_DESCRIPTOR_TYPE_SAMPLER,
		1,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		nullptr
	});



	/* Create the descriptor layout. */
	VulkanHelpers::CreateDescriptorSetLayout(_device, &_descLayout, bindings.size(), bindings.data());



	/* Allocate the desciptor set*/
	VulkanHelpers::AllocateDescriptorSets(_device, _descPool, 1, &_descLayout, &_descSet);




	std::vector<VkWriteDescriptorSet> WriteDS;

	auto& bufferInfo = _vertexBufferHandler->GetBufferInfo();
	for (uint32_t i = 0; i < bufferInfo.size(); i++) {
		WriteDS.push_back(VulkanHelpers::MakeWriteDescriptorSet(_descSet, i, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &bufferInfo[i], nullptr));
	}


	/*Update the descriptor set with the binding data*/
	vkUpdateDescriptorSets(_device, WriteDS.size(), WriteDS.data(), 0, nullptr);



}

void Renderer::_CreateVPUniformBuffer()
{

	VkDeviceSize size = sizeof(VPUniformBuffer);
	VulkanHelpers::CreateBuffer(_devices[0], _device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &_VPUniformBufferStaging, &_VPUniformBufferMemoryStaging);
	VulkanHelpers::CreateBuffer(_devices[0], _device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_VPUniformBuffer, &_VPUniformBufferMemory);

	//Stick an identity matrix into as default
	void* src;
	VulkanHelpers::MapMemory(_device, _VPUniformBufferMemoryStaging, &src, sizeof(VPUniformBuffer));
	VPUniformBuffer def; 
	memcpy(src, &def, sizeof(VPUniformBuffer));
	vkUnmapMemory(_device, _VPUniformBufferMemoryStaging);

	VulkanHelpers::BeginCommandBuffer(_cmdBuffer);
	VulkanHelpers::CopyDataBetweenBuffers(_cmdBuffer, _VPUniformBufferStaging, 0, _VPUniformBuffer, 0, sizeof(VPUniformBuffer));
	vkEndCommandBuffer(_cmdBuffer);
	auto& sInfo = VulkanHelpers::MakeSubmitInfo(1, &_cmdBuffer);
	VulkanHelpers::QueueSubmit(_queue, 1, &sInfo);
	vkQueueWaitIdle(_queue);


}


