#include "VulkanHelpers.h"




VkResult VulkanHelpers::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugReportCallbackEXT * pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanHelpers::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

VkApplicationInfo VulkanHelpers::MakeApplicationInfo(const char * pApplicationName, uint32_t applicationVersion, const char * pEngineName, uint32_t engineVersion, uint32_t apiVersion, void * pNext)
{
	VkApplicationInfo vkAppInfo =
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,	// sType;
		pNext,// pNext;
		pApplicationName,// Application name
		applicationVersion,// applicationVersion;
		pEngineName,// pEngineName;
		engineVersion,// engineVersion;
		apiVersion// apiVersion;
	};
	return vkAppInfo;
}

VkInstanceCreateInfo VulkanHelpers::MakeInstanceCreateInfo(VkInstanceCreateFlags flags, const VkApplicationInfo * pApplicationInfo, uint32_t enabledLayerCount, const char * const * ppEnabledLayerNames, void * pNext, uint32_t enabledExtensionCount, const char * const * ppEnabledExtensionNames)
{
	VkInstanceCreateInfo vkInstCreateInfo =
	{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,//sType
		pNext,//pNext
		flags,//flags	
		pApplicationInfo,//pApplicationInfo
		enabledLayerCount,//enabledLayerCount
		ppEnabledLayerNames,//ppEnabledLayerNames
		enabledExtensionCount,// enabledExtCount
		ppEnabledExtensionNames// ppEnabledExtNames
	};

	return vkInstCreateInfo;
}

VkDeviceQueueCreateInfo VulkanHelpers::MakeDeviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t queueCount, const float * pQueuePriorities, VkDeviceQueueCreateFlags flags, void * pNext)
{
	const VkDeviceQueueCreateInfo deviceQueueCreateInfo =
	{
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,   // sType
		pNext,                                        // pNext
		flags,                                        // flags
		queueFamilyIndex,                             // queueFamilyIndex
		queueCount,                                   // queueCount
		pQueuePriorities                              // pQueuePriorities
	};

	return deviceQueueCreateInfo;
}

VkDeviceCreateInfo VulkanHelpers::MakeDeviceCreateInfo(uint32_t queueCreateInfoCount, const VkDeviceQueueCreateInfo * pQueueCreateInfos, uint32_t enabledLayerCount, const char * const * ppEnabledLayerNames, const VkPhysicalDeviceFeatures * pEnabledFeatures, void * pNext, uint32_t enabledExtensionCount, const char * const * ppEnabledExtensionNames, VkDeviceCreateFlags flags)
{
	const VkDeviceCreateInfo deviceCreateInfo =
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,     // sType
		pNext,                                    // pNext
		flags,                                    // flags
		queueCreateInfoCount,                     // queueCreateInfoCount
		pQueueCreateInfos,						  // pQueueCreateInfos
		enabledLayerCount,                        // enabledLayerCount
		ppEnabledLayerNames,                      // ppEnabledLayerNames
		enabledExtensionCount,                    // enabledExtensionCount
		ppEnabledExtensionNames,                  // ppEnabledExtensionNames
		pEnabledFeatures						  // pEnabledFeatures
	};
	return deviceCreateInfo;
}

VkCommandPoolCreateInfo VulkanHelpers::MakeCommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags, void * pNext)
{
	VkCommandPoolCreateInfo cmdPoolInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		pNext,
		flags,
		queueFamilyIndex
	};

	return cmdPoolInfo;
}



VkSubmitInfo VulkanHelpers::MakeSubmitInfo(uint32_t commandBufferCount, const VkCommandBuffer * pCommandBuffers, uint32_t waitSemaphoreCount, const VkSemaphore * pWaitSemaphores, const VkPipelineStageFlags * pWaitDstStageMask, uint32_t signalSemaphoreCount, const VkSemaphore * pSignalSemaphores, void * pNext)
{
	VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		pNext,
		waitSemaphoreCount,
		pWaitSemaphores,
		pWaitDstStageMask,
		commandBufferCount,
		pCommandBuffers,
		signalSemaphoreCount,
		pSignalSemaphores
	};

	return submitInfo;
}

VkMappedMemoryRange VulkanHelpers::MakeMappedMemoryRange(VkDeviceMemory memory, VkDeviceSize size, VkDeviceSize offset, void * pNext)
{
	VkMappedMemoryRange range =
	{
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		pNext,
		memory,
		offset,
		size
	};

	return range;
}

VkBufferCreateInfo VulkanHelpers::MakeBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage, VkBufferCreateFlags flags, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount, const uint32_t * pQueueFamilyIndices, void * pNext)
{
	VkBufferCreateInfo info =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		pNext,
		flags,
		size,
		usage,
		sharingMode,
		queueFamilyIndexCount,
		pQueueFamilyIndices
	};

	return info;
}

VkImageCreateInfo VulkanHelpers::MakeImageCreateInfo(VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkImageLayout initialLayout, VkSampleCountFlagBits samples, uint32_t arrayLayers, VkImageType imageType, uint32_t mipLevels, VkImageCreateFlags flags, VkImageTiling tiling, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount, const uint32_t * pQueueFamilyIndices, void * pNext)
{
	VkImageCreateInfo imageCreateInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,        // sType
		pNext,                                    // pNext
		flags,                                          // flags
		imageType,                           // imageType
		format,                   // format
		extent,                          // extent
		mipLevels,                                         // mipLevels
		arrayLayers,                                          // arrayLayers
		samples,                      // samples
		tiling,                    // tiling
		usage,                 // usage
		sharingMode,                  // sharingMode
		queueFamilyIndexCount,                                          // queueFamilyIndexCount
		pQueueFamilyIndices,                                    // pQueueFamilyIndices
		initialLayout                   // initialLayout
	};
	return imageCreateInfo;
}

VkWriteDescriptorSet VulkanHelpers::MakeWriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount, VkDescriptorType descriptorType, const VkDescriptorImageInfo * pImageInfo, const VkDescriptorBufferInfo * pBufferInfo, const VkBufferView * pTexelBufferView, void * pNext)
{
	VkWriteDescriptorSet info = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		dstSet,
		dstBinding,
		dstArrayElement,
		descriptorCount,
		descriptorType,
		pImageInfo,
		pBufferInfo,
		pTexelBufferView
	};

	return info;
}

VkGraphicsPipelineCreateInfo VulkanHelpers::MakePipelineCreateInfo(uint32_t stageCount, const VkPipelineShaderStageCreateInfo * pStages, const VkPipelineVertexInputStateCreateInfo * pVertexInputState, const VkPipelineInputAssemblyStateCreateInfo * pInputAssemblyState, const VkPipelineTessellationStateCreateInfo * pTessellationState, const VkPipelineViewportStateCreateInfo * pViewportState, const VkPipelineRasterizationStateCreateInfo * pRasterizationState, const VkPipelineMultisampleStateCreateInfo * pMultisampleState, const VkPipelineDepthStencilStateCreateInfo * pDepthStencilState, const VkPipelineColorBlendStateCreateInfo * pColorBlendState, const VkPipelineDynamicStateCreateInfo * pDynamicState, VkPipelineLayout layout, VkRenderPass renderPass, uint32_t subpass, VkPipeline basePipelineHandle, int32_t basePipelineIndex, VkPipelineCreateFlags flags, void * pNext)
{
	return{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		pNext,
		flags,
		stageCount,
		pStages,
		pVertexInputState,
		pInputAssemblyState,
		pTessellationState,
		pViewportState,
		pRasterizationState,
		pMultisampleState,
		pDepthStencilState,
		pColorBlendState,
		pDynamicState,
		layout,
		renderPass,
		subpass,
		basePipelineHandle,
		basePipelineIndex
	};
}

VkSamplerCreateInfo VulkanHelpers::MakeSamplerCreateInfo(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW, float mipLodBias, VkBool32 anisotropyEnable, float maxAnisotropy, VkBool32 compareEnable, VkCompareOp compareOp, float minLod, float maxLod, VkBorderColor borderColor, VkBool32 unnormalizedCoordinates, VkSamplerCreateFlags flags, void * pNext)
{
	return{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		pNext,
		flags,
		magFilter,
		minFilter,
		mipmapMode,
		addressModeU,
		addressModeV,
		addressModeW,
		mipLodBias,
		anisotropyEnable,
		maxAnisotropy,
		compareEnable,
		compareOp,
		minLod,
		maxLod,
		borderColor,
		unnormalizedCoordinates
	};
}

VkQueryPoolCreateInfo VulkanHelpers::MakeQueryPoolCreateInfo(VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics, VkQueryPoolCreateFlags flags, void * pNext)
{
	VkQueryPoolCreateInfo info =
	{
		VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		pNext,
		flags,
		queryType,
		queryCount,
		pipelineStatistics
	};
	return info;
}


void VulkanHelpers::CreateInstance(const VkInstanceCreateInfo * pCreateInfo, VkInstance * pInstance, const VkAllocationCallbacks * pAllocator)
{
	/******** Create the instance***********/
	VkResult result = vkCreateInstance(pCreateInfo, pAllocator, pInstance);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance.");
	}
}

void VulkanHelpers::CreateLogicDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo * pCreateInfo, VkDevice * pDevice, const VkAllocationCallbacks * pAllocator)
{
	VkResult	result = vkCreateDevice(physicalDevice,
		pCreateInfo,
		pAllocator,
		pDevice);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logic device.");
	}
}

void VulkanHelpers::CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo * pCreateInfo, VkCommandPool * pCommandPool, const VkAllocationCallbacks * pAllocator)
{
	VkResult result = vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);


	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool.");
	}
}

void VulkanHelpers::AllocateCommandBuffers(VkDevice device, VkCommandBuffer * pCommandBuffers, VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t commandBufferCount, void * pNext)
{
	VkCommandBufferAllocateInfo cmdBufferAllocInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		pNext,
		commandPool,
		level,
		commandBufferCount
	};

	VkResult result = vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, pCommandBuffers);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers.");
	}
}

void VulkanHelpers::CreateBuffer(VkDevice device, const VkBufferCreateInfo * pCreateInfo, VkBuffer * pBuffer, const VkAllocationCallbacks * pAllocator)
{
	VkResult result = vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer.");
	}
}

void VulkanHelpers::CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer * buffer, VkDeviceMemory * bufferMemory)
{
	/*Create the buffer*/
	const auto bufferInfo = &VulkanHelpers::MakeBufferCreateInfo(
		size,
		usage);
	VulkanHelpers::CreateBuffer(device, bufferInfo, buffer);


	/*Get memory requirments*/
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(device, *buffer, &memReq);
	auto memTypeIndex = VulkanHelpers::ChooseHeapFromFlags(physicalDevice, &memReq, properties, properties);


	/*Allocate the memory*/
	VulkanHelpers::AllocateMemory(
		device,
		memReq.size,
		memTypeIndex,
		bufferMemory
	);

	/*Bind the memory to the buffer*/
	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}


void VulkanHelpers::CreateImage(VkDevice device, const VkImageCreateInfo * pCreateInfo, VkImage* pImage, const VkAllocationCallbacks * pAllocator)
{
	VkResult result = vkCreateImage(device, pCreateInfo, pAllocator, pImage);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer.");
	}
}

void VulkanHelpers::CreateImage2D(VkDevice device, VkImage * image, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage)
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	VulkanHelpers::CreateImage(device, &imageCreateInfo, image, nullptr); //Throws if failed
}

void VulkanHelpers::AllocateImageMemory(VkDevice device, VkPhysicalDevice physDevice, VkImage image, VkMemoryPropertyFlags propertyFlags, VkDeviceMemory * memory)
{
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, image, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &memoryProperties);

	uint32_t memoryTypeIndex = 0;
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags))
		{
			memoryTypeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, memory) != VK_SUCCESS)
		throw std::runtime_error(std::string("Could not allocate memory for staging image"));

}

void VulkanHelpers::TransitionImageLayout(VkDevice device, VkImage image, VkCommandBuffer cmdBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanHelpers::CreateDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout * pSetLayout, uint32_t bindingCount, const VkDescriptorSetLayoutBinding * pBindings, VkDescriptorSetLayoutCreateFlags flags, const VkAllocationCallbacks * pAllocator, void * pNext)
{
	VkDescriptorSetLayoutCreateInfo info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		pNext,
		flags,
		bindingCount,
		pBindings
	};

	VkResult r = vkCreateDescriptorSetLayout(device, &info, pAllocator, pSetLayout);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Descriptor set layout");
	}
}


void VulkanHelpers::CreatePipelineLayout(VkDevice device, VkPipelineLayout * pPipelineLayout, uint32_t setLayoutCount, const VkDescriptorSetLayout * pSetLayouts, uint32_t pushConstantRangeCount, const VkPushConstantRange * pPushConstantRanges, const VkAllocationCallbacks * pAllocator, void * pNext, VkPipelineLayoutCreateFlags flags)
{
	VkPipelineLayoutCreateInfo info = {
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		pNext,
		flags,
		setLayoutCount,
		pSetLayouts,
		pushConstantRangeCount,
		pPushConstantRanges
	};

	VkResult r = vkCreatePipelineLayout(device, &info, pAllocator, pPipelineLayout);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

}

void VulkanHelpers::CreateDescriptorPool(VkDevice device, VkDescriptorPool * pDescriptorPool, VkDescriptorPoolCreateFlags flags, uint32_t maxSets, uint32_t poolSizeCount, const VkDescriptorPoolSize * pPoolSizes, const VkAllocationCallbacks * pAllocator, void * pNext)
{

	VkDescriptorPoolCreateInfo info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		pNext,
		flags,
		maxSets,
		poolSizeCount,
		pPoolSizes
	};

	VkResult r = vkCreateDescriptorPool(device, &info, pAllocator, pDescriptorPool);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void VulkanHelpers::AllocateDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSetLayout * pSetLayouts, VkDescriptorSet * pDescriptorSets, void * pNext)
{
	VkDescriptorSetAllocateInfo info = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		pNext,
		descriptorPool,
		descriptorSetCount,
		pSetLayouts
	};

	VkResult r = vkAllocateDescriptorSets(device, &info, pDescriptorSets);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("Failed to create allocate descriptor sets");
	}

}

void VulkanHelpers::CreateBufferView(VkDevice device, VkBuffer buffer, VkBufferView * pView, VkFormat format, VkDeviceSize offset, VkDeviceSize range, VkBufferViewCreateFlags flags, void * pNext, const VkAllocationCallbacks * pAllocator)
{
	VkBufferViewCreateInfo info = {
		VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
		pNext,
		flags,
		buffer,
		format,
		offset,
		range
	};

	VkResult r = vkCreateBufferView(device, &info, pAllocator, pView);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer view");
	}

}

void VulkanHelpers::CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo * pCreateInfos, VkPipeline * pPipelines, const VkAllocationCallbacks * pAllocator)
{
	VkResult r = vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("Could not create graphics pipeline");
	}
}

void VulkanHelpers::CreateSampler(VkDevice device, const VkSamplerCreateInfo * pCreateInfo, VkSampler * pSampler, const VkAllocationCallbacks * pAllocator)
{
	VkResult r = vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("Could not create sampler");
	}
}

void VulkanHelpers::CreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo * pCreateInfo, VkQueryPool * pQueryPool, const VkAllocationCallbacks * pAllocator)
{
	VkResult r = vkCreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
	if (r != VK_SUCCESS) {
		throw std::runtime_error("Could not create query pool");
	}
}

void VulkanHelpers::BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags, const VkCommandBufferInheritanceInfo * pInheritanceInfo, void* pNext)
{
	/*typedef struct VkCommandBufferBeginInfo {
    VkStructureType                        sType;
    void*                            pNext;
    VkCommandBufferUsageFlags              flags;
    const VkCommandBufferInheritanceInfo*  pInheritanceInfo;
} VkCommandBufferBeginInfo;*/

	VkCommandBufferBeginInfo cmdInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		pNext,
		flags,
		pInheritanceInfo
	};

	VkResult result = vkBeginCommandBuffer(commandBuffer, &cmdInfo);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to being command buffer");
	}
}

void VulkanHelpers::EndCommandBuffer(VkCommandBuffer commandBuffer)
{
	VkResult result = vkEndCommandBuffer(commandBuffer);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to end command buffer");
	}
}

void VulkanHelpers::ResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags)
{
	VkResult result = vkResetCommandBuffer(commandBuffer, flags);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to reset command buffer");
	}
}

void VulkanHelpers::ResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
	VkResult result = vkResetCommandPool(device, commandPool, flags);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to reset command pool");
	}
}

void VulkanHelpers::QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo * pSubmits, VkFence fence)
{
	VkResult result = vkQueueSubmit(queue, submitCount, pSubmits, fence);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit queue.");
	}
}

void VulkanHelpers::AllocateMemory(VkDevice device, VkDeviceSize allocationSize, uint32_t memoryTypeIndex, VkDeviceMemory * pMemory, void * pNext, const VkAllocationCallbacks * pAllocator)
{
	VkMemoryAllocateInfo allocInfo =
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		pNext,
		allocationSize,
		memoryTypeIndex
	};


	VkResult result = vkAllocateMemory(device, &allocInfo, pAllocator, pMemory);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate memory.");
	}
}

void VulkanHelpers::MapMemory(VkDevice device, VkDeviceMemory memory, void ** ppData, VkDeviceSize size, VkDeviceSize offset, VkMemoryMapFlags flags)
{
	VkResult result = vkMapMemory(device, memory, offset, size, flags, ppData);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to map memory.");
	}
}

void VulkanHelpers::CopyDataBetweenBuffers(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkDeviceSize srcOffset, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size)
{
	const VkBufferCopy copyRegion =
	{
		srcOffset, dstOffset, size
	};

	vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}


uint32_t VulkanHelpers::ChooseHeapFromFlags(VkPhysicalDevice physicalDevice, const VkMemoryRequirements * memoryRequirements, VkMemoryPropertyFlags requiredFlags, VkMemoryPropertyFlags preferredFlags)
{
	const auto& deviceMemoryProperties = GetPhysicalDeviceMemoryProperties(physicalDevice);

	uint32_t selectedType = ~0u;
	uint32_t memoryType;

	for (memoryType = 0; memoryType < 32; ++memoryType)
	{
		if (memoryRequirements->memoryTypeBits & (1 << memoryType))
		{
			const VkMemoryType& type =
				deviceMemoryProperties.memoryTypes[memoryType];

			// If it exactly matches my preferred properties, grab it.
			if ((type.propertyFlags & preferredFlags) == preferredFlags)
			{
				selectedType = memoryType;
				break;
			}
		}
	}
	if (selectedType != ~0u)
	{
		for (memoryType = 0; memoryType < 32; ++memoryType)
		{
			if (memoryRequirements->memoryTypeBits & (1 << memoryType))
			{
				const VkMemoryType& type =
					deviceMemoryProperties.memoryTypes[memoryType];

				// If it has all my required properties, it'll do.
				if ((type.propertyFlags & requiredFlags) == requiredFlags)
				{
					selectedType = memoryType;
					break;
				}
			}
		}
	}

	return selectedType;
}

VkSubresourceLayout VulkanHelpers::GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource * pSubresource)
{
	VkSubresourceLayout layout;

	vkGetImageSubresourceLayout(device, image, pSubresource, &layout);

	return layout;
}

std::vector<VkPhysicalDevice> VulkanHelpers::EnumeratePhysicalDevices(VkInstance instance)
{
	std::vector<VkPhysicalDevice> physicalDevices;
	// First figure out how many devices are in the system.
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);


	// Size the device array appropriately and get the physical
	// device handles.
	physicalDevices.resize(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance,
		&physicalDeviceCount,
		&physicalDevices[0]);

	return physicalDevices;
}

VkPhysicalDeviceProperties VulkanHelpers::GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties properties;

	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	return properties;
}

VkPhysicalDeviceMemoryProperties VulkanHelpers::GetPhysicalDeviceMemoryProperties(VkPhysicalDevice phydev)
{
	VkPhysicalDeviceMemoryProperties prop;

	vkGetPhysicalDeviceMemoryProperties(phydev, &prop);

	return prop;
}

VkPhysicalDeviceFeatures VulkanHelpers::GetPhysicalDeviceFeatures(VkPhysicalDevice phydev)
{
	VkPhysicalDeviceFeatures feat;

	vkGetPhysicalDeviceFeatures(phydev, &feat);

	return feat;
}

std::vector<VkPhysicalDeviceFeatures> VulkanHelpers::EnumeratePhysicalDeviceFeatures(VkInstance instance)
{
	std::vector<VkPhysicalDeviceFeatures> features;

	auto& phydevs = EnumeratePhysicalDevices(instance);

	for (auto& pd : phydevs)
	{
		features.push_back(GetPhysicalDeviceFeatures(pd));
	}

	return features;
}

std::vector<VkQueueFamilyProperties> VulkanHelpers::EnumeratePhysicalDeviceQueueFamilyProperties(VkPhysicalDevice phydev)
{
	uint32_t queueFamilyPropertyCount;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;

	// First determine the number of queue families supported by the physical
	// device.
	vkGetPhysicalDeviceQueueFamilyProperties(phydev,
		&queueFamilyPropertyCount,
		nullptr);

	// Allocate enough space for the queue property structures.
	queueFamilyProperties.resize(queueFamilyPropertyCount);

	// Now query the actual properties of the queue families.
	vkGetPhysicalDeviceQueueFamilyProperties(phydev,
		&queueFamilyPropertyCount,
		queueFamilyProperties.data());

	return queueFamilyProperties;
}

std::vector<std::vector<VkQueueFamilyProperties>> VulkanHelpers::EnumeratePhysicalDeviceQueueFamilyProperties(VkInstance instance)
{
	std::vector<std::vector<VkQueueFamilyProperties>> propDev;

	auto& devices = EnumeratePhysicalDevices(instance);

	for (auto& d : devices)
	{
		propDev.push_back(EnumeratePhysicalDeviceQueueFamilyProperties(d));
	}


	return propDev;
}

VkFormatProperties VulkanHelpers::GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format)
{
	VkFormatProperties prop;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &prop);
	return prop;
}

VkImageFormatProperties VulkanHelpers::GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags)
{
	VkImageFormatProperties prop;
	VkResult result = vkGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, &prop);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to get Physical Device Image Format Properties.");
	}

	return prop;
}

void VulkanHelpers::PrintPhysicalDeviceProperties(VkPhysicalDeviceProperties prop)
{
	printf("\tDevice Name: %s\n", prop.deviceName);
	printf("\tDevice Driver Version: %d\n", prop.driverVersion);
	printf("***********************************\n\n");
}

void VulkanHelpers::PrintQueueFamilyProperties(VkQueueFamilyProperties fam)
{
	printf("\tQueue Flags: \n");
	if (fam.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		printf("\t\t VK_QUEUE_GRAPHICS_BIT\n");
	if (fam.queueFlags & VK_QUEUE_COMPUTE_BIT)
		printf("\t\t VK_QUEUE_COMPUTE_BIT\n");
	if (fam.queueFlags & VK_QUEUE_TRANSFER_BIT)
		printf("\t\t VK_QUEUE_TRANSFER_BIT\n");
	if (fam.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
		printf("\t\t VK_QUEUE_SPARSE_BINDING_BIT\n");

	printf("\tQueue Count: %I32d\n", fam.queueCount);
	printf("***********************************\n\n");
}

void VulkanHelpers::PrintPhysicalDeviceMemoryProperties(VkPhysicalDeviceMemoryProperties prop)
{
	printf("\tMemory Type Count: %I32d\n", prop.memoryTypeCount);
	for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
	{
		auto& memType = prop.memoryTypes[i];
		printf("\t\tHeap Index: %I32d\n", memType.heapIndex);
		printf("\t\tPropertyFlags: \n");
		if (memType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			printf("\t\t\tVK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n");

		if (memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			printf("\t\t\tVK_MEMORY_PROPERTY_HOST_VISIBLE_BIT\n");

		if (memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			printf("\t\t\tVK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n");

		if (memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
			printf("\t\t\tVK_MEMORY_PROPERTY_HOST_CACHED_BIT\n");

		if (memType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
			printf("\t\t\tVK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT\n");
		printf("------------------------------------\n\n");
	}

	printf("\tMemory Heap Count: %I32d\n", prop.memoryHeapCount);
	for (uint32_t i = 0; i < prop.memoryHeapCount; i++)
	{
		auto& memHeap = prop.memoryHeaps[i];
		printf("\t\tHeap Size: %I64d\n", memHeap.size);
		printf("\t\tPropertyFlags: \n");
		if (memHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			printf("\t\t\tVK_MEMORY_HEAP_DEVICE_LOCAL_BIT\n");
		printf("------------------------------------\n\n");
	}

	printf("***********************************\n\n");
}

