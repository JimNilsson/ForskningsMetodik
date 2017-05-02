#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan\vulkan.h>
#include <vector>

#pragma comment(lib,"vulkan-1.lib")

#pragma once
#define MB *1024*1024
namespace VulkanHelpers
{

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData) {

		printf("validation layer: %s\n\n", msg);
		return VK_FALSE;
	}


	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	/*Abstract allocator class*/
	class DeviceAllocator
	{
	protected:
		DeviceAllocator() {};
	public:
		~DeviceAllocator() {};

		virtual void Allocate(VkDeviceSize size) = 0;
	};


	/*Struct creation*/
	VkApplicationInfo MakeApplicationInfo(
		const char*									pApplicationName			= nullptr,
		uint32_t           							applicationVersion			= VK_MAKE_VERSION(0,0,0),
		const char*        							pEngineName					= nullptr,
		uint32_t           							engineVersion				= VK_MAKE_VERSION(0,0,0),
		uint32_t           							apiVersion					= VK_API_VERSION_1_0,
		void*        							pNext						= nullptr);

	VkInstanceCreateInfo MakeInstanceCreateInfo(
		VkInstanceCreateFlags						flags,
		const VkApplicationInfo*					pApplicationInfo			= nullptr,
		uint32_t									enabledLayerCount			= 0,
		const char* const*							ppEnabledLayerNames			= nullptr,
		void*									pNext						= nullptr,
		uint32_t									enabledExtensionCount		= 0,
		const char* const*							ppEnabledExtensionNames		= nullptr);

	VkDeviceQueueCreateInfo MakeDeviceQueueCreateInfo(		
		uint32_t									queueFamilyIndex,
		uint32_t									queueCount,
		const float*								pQueuePriorities			= nullptr,
		VkDeviceQueueCreateFlags					flags						= 0,
		void*									pNext						= nullptr);

	VkDeviceCreateInfo MakeDeviceCreateInfo(	
		uint32_t									queueCreateInfoCount		= 0,
		const VkDeviceQueueCreateInfo*				pQueueCreateInfos			= nullptr,
		uint32_t									enabledLayerCount			= 0,
		const char* const*							ppEnabledLayerNames			= nullptr,
		const VkPhysicalDeviceFeatures*				pEnabledFeatures			= nullptr,
		void*									pNext						= nullptr,
		uint32_t									enabledExtensionCount		= 0,
		const char* const*							ppEnabledExtensionNames		= nullptr,
		VkDeviceCreateFlags							flags						= 0);

	VkCommandPoolCreateInfo MakeCommandPoolCreateInfo(
		uint32_t									queueFamilyIndex,
		VkCommandPoolCreateFlags					flags						= 0,
		void*									pNext						= nullptr);

	VkSubmitInfo MakeSubmitInfo(
		uint32_t									commandBufferCount,
		const VkCommandBuffer*						pCommandBuffers,
		uint32_t									waitSemaphoreCount			= 0,
		const VkSemaphore*							pWaitSemaphores				= nullptr,
		const VkPipelineStageFlags*					pWaitDstStageMask			= nullptr,
		uint32_t									signalSemaphoreCount		= 0,
		const VkSemaphore*							pSignalSemaphores			= nullptr,
		void*									pNext						= nullptr);

	VkMappedMemoryRange MakeMappedMemoryRange(
		VkDeviceMemory								memory,
		VkDeviceSize								size						= 0,
		VkDeviceSize      							offset						= VK_WHOLE_SIZE,
		void*        							pNext						= nullptr);
	
	VkBufferCreateInfo MakeBufferCreateInfo(
		VkDeviceSize								size,
		VkBufferUsageFlags     						usage,
		VkBufferCreateFlags    						flags						= 0,
		VkSharingMode          						sharingMode					= VK_SHARING_MODE_EXCLUSIVE,
		uint32_t               						queueFamilyIndexCount		= 0,
		const uint32_t*        						pQueueFamilyIndices			= nullptr,
		void*            						pNext						= nullptr);




	VkImageCreateInfo MakeImageCreateInfo(
		VkFormat									format,
		VkExtent3D               					extent,
		VkImageUsageFlags        					usage,
		VkImageLayout            					initialLayout				 = VK_IMAGE_LAYOUT_UNDEFINED,
		VkSampleCountFlagBits    					samples						 = VK_SAMPLE_COUNT_1_BIT,
		uint32_t                 					arrayLayers					 = 1,
		VkImageType              					imageType					 = VK_IMAGE_TYPE_2D,
		uint32_t                 					mipLevels					 = 10,
		VkImageCreateFlags       					flags						 = 0,
		VkImageTiling            					tiling						 = VK_IMAGE_TILING_OPTIMAL,
		VkSharingMode            					sharingMode					 = VK_SHARING_MODE_EXCLUSIVE,
		uint32_t                 					queueFamilyIndexCount		 = 0,
		const uint32_t*          					pQueueFamilyIndices			 = nullptr,
		void*              					pNext						 = nullptr);


	VkWriteDescriptorSet MakeWriteDescriptorSet(
		VkDescriptorSet								dstSet,
		uint32_t									dstBinding,
		uint32_t									dstArrayElement,
		uint32_t									descriptorCount,
		VkDescriptorType							descriptorType,
		const VkDescriptorImageInfo*				pImageInfo,
		const VkDescriptorBufferInfo*				pBufferInfo,
		const VkBufferView*							pTexelBufferView,
		void*									pNext						= nullptr);


	VkGraphicsPipelineCreateInfo MakePipelineCreateInfo(
		uint32_t                                         stageCount,
		const VkPipelineShaderStageCreateInfo*           pStages,
		const VkPipelineVertexInputStateCreateInfo*      pVertexInputState,
		const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState,
		const VkPipelineTessellationStateCreateInfo*     pTessellationState,
		const VkPipelineViewportStateCreateInfo*         pViewportState,
		const VkPipelineRasterizationStateCreateInfo*    pRasterizationState,
		const VkPipelineMultisampleStateCreateInfo*      pMultisampleState,
		const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState,
		const VkPipelineColorBlendStateCreateInfo*       pColorBlendState,
		const VkPipelineDynamicStateCreateInfo*          pDynamicState,
		VkPipelineLayout                                 layout,
		VkRenderPass                                     renderPass,
		uint32_t                                         subpass				= 0,
		VkPipeline                                       basePipelineHandle		= VK_NULL_HANDLE,
		int32_t                                          basePipelineIndex		= 0,
		VkPipelineCreateFlags                            flags					= 0,
		void*                                      pNext					= nullptr);

	VkSamplerCreateInfo MakeSamplerCreateInfo(
		VkFilter										magFilter,
		VkFilter										minFilter,
		VkSamplerMipmapMode								mipmapMode,
		VkSamplerAddressMode							addressModeU,
		VkSamplerAddressMode							addressModeV,
		VkSamplerAddressMode							addressModeW,
		float											mipLodBias,
		VkBool32										anisotropyEnable,
		float											maxAnisotropy,
		VkBool32										compareEnable,
		VkCompareOp										compareOp,
		float											minLod,
		float											maxLod,
		VkBorderColor									borderColor,
		VkBool32										unnormalizedCoordinates,
		VkSamplerCreateFlags							flags					= 0,
		void*										pNext					= nullptr);

	VkQueryPoolCreateInfo MakeQueryPoolCreateInfo(
		VkQueryType										queryType,
		uint32_t										queryCount,
		VkQueryPipelineStatisticFlags					pipelineStatistics		= 0,
		VkQueryPoolCreateFlags							flags					= 0,
		void*										pNext					= nullptr);

	/*Creation*/
	void CreateInstance(
		const VkInstanceCreateInfo*                 pCreateInfo,
		VkInstance*                                 pInstance,
		const VkAllocationCallbacks*                pAllocator					= nullptr);


	void CreateLogicDevice(
		VkPhysicalDevice                            physicalDevice,
		const VkDeviceCreateInfo*                   pCreateInfo,
		VkDevice*                                   pDevice,
		const VkAllocationCallbacks*                pAllocator					= nullptr);

	void CreateCommandPool(
		VkDevice									device,
		const VkCommandPoolCreateInfo*				pCreateInfo,
		VkCommandPool*								pCommandPool,
		const VkAllocationCallbacks*				pAllocator					= nullptr);

	void AllocateCommandBuffers(
		VkDevice									device,
		VkCommandBuffer*							pCommandBuffers, 
		VkCommandPool								commandPool,
		VkCommandBufferLevel						level						= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		uint32_t									commandBufferCount			= 1,
		void*									pNext						= nullptr);

	void CreateBuffer(
		VkDevice									device,
		const VkBufferCreateInfo*					pCreateInfo,
		VkBuffer*									pBuffer,
		const VkAllocationCallbacks*				pAllocator					= nullptr);

	void CreateBuffer(
		VkPhysicalDevice                            physicalDevice,
		VkDevice									device,
		VkDeviceSize								size,
		VkBufferUsageFlags							usage,
		VkMemoryPropertyFlags						properties,
		VkBuffer* buffer, VkDeviceMemory*			bufferMemory);

	void CreateImage(
		VkDevice									device,
		const VkImageCreateInfo*					pCreateInfo,
		VkImage*									pImage,
		const VkAllocationCallbacks*				pAllocator					= nullptr);

	void CreateImage2D(
		VkDevice									device,
		VkImage*									image,
		uint32_t									width,
		uint32_t									height,
		VkFormat									format,
		VkImageTiling								tiling,
		VkImageUsageFlags							usage );

	void AllocateImageMemory(
		VkDevice									device,
		VkPhysicalDevice							physDevice,
		VkImage										image,
		VkMemoryPropertyFlags						propertyFlags,
		VkDeviceMemory*								memory);

	void TransitionImageLayout(
		VkDevice									device,
		VkImage										image,
		VkCommandBuffer								cmdBuffer,
		VkFormat									format,
		VkImageLayout								oldLayout,
		VkImageLayout								newLayout
	);


	void CreateDescriptorSetLayout(
		VkDevice									device,
		VkDescriptorSetLayout*						pSetLayout,
		uint32_t									bindingCount,
		const VkDescriptorSetLayoutBinding*			pBindings,
		VkDescriptorSetLayoutCreateFlags			flags						= 0,
		const VkAllocationCallbacks*				pAllocator					= nullptr,
		void*									pNext						= nullptr);

	void CreatePipelineLayout(
		VkDevice									device,
		VkPipelineLayout*							pPipelineLayout,
		uint32_t									setLayoutCount,
		const VkDescriptorSetLayout*				pSetLayouts,
		uint32_t									pushConstantRangeCount		= 0,
		const VkPushConstantRange*					pPushConstantRanges			= 0,
		const VkAllocationCallbacks*				pAllocator					= nullptr,
		void*									pNext						= nullptr,
		VkPipelineLayoutCreateFlags					flags						= 0);


	void CreateDescriptorPool(
		VkDevice									device,
		VkDescriptorPool*							pDescriptorPool, 
		VkDescriptorPoolCreateFlags					flags,
		uint32_t									maxSets,
		uint32_t									poolSizeCount,
		const VkDescriptorPoolSize*					pPoolSizes,
		const VkAllocationCallbacks*				pAllocator					= nullptr,
		void*									pNext						= nullptr);

	void AllocateDescriptorSets(
		VkDevice									device,
		VkDescriptorPool							descriptorPool,
		uint32_t									descriptorSetCount,
		const VkDescriptorSetLayout*				pSetLayouts,
		VkDescriptorSet*							pDescriptorSets,
		void*									pNext						= nullptr);

	void CreateBufferView(
		VkDevice                                    device,
		VkBuffer									buffer,
		VkBufferView*                               pView,
		VkFormat									format,
		VkDeviceSize								offset						= 0,
		VkDeviceSize								range						= VK_WHOLE_SIZE,
		VkBufferViewCreateFlags						flags						= 0,
		void*									pNext						= nullptr,	
		const VkAllocationCallbacks*                pAllocator					= nullptr);

	void CreateGraphicsPipelines(
		VkDevice                                    device,
		VkPipelineCache                             pipelineCache,
		uint32_t                                    createInfoCount,
		const VkGraphicsPipelineCreateInfo*         pCreateInfos,
		VkPipeline*                                 pPipelines,
		const VkAllocationCallbacks*                pAllocator					= nullptr );

	void CreateSampler(
		VkDevice                                    device,
		const VkSamplerCreateInfo*                  pCreateInfo,
		VkSampler*                                  pSampler,
		const VkAllocationCallbacks*                pAllocator					= nullptr);

	void CreateQueryPool(
		VkDevice                                    device,
		const VkQueryPoolCreateInfo*                pCreateInfo,
		VkQueryPool*                                pQueryPool,
		const VkAllocationCallbacks*                pAllocator					= nullptr);

	/*Command recording*/
	void BeginCommandBuffer(
		VkCommandBuffer								commandBuffer,
		VkCommandBufferUsageFlags					flags						= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		const VkCommandBufferInheritanceInfo*		pInheritanceInfo			= nullptr,
		void*									pNext						= nullptr);

	void EndCommandBuffer(
		VkCommandBuffer								commandBuffer);

	void ResetCommandBuffer(
		VkCommandBuffer								commandBuffer,
		VkCommandBufferResetFlags					flags);

	void ResetCommandPool(
		VkDevice									device,
		VkCommandPool								commandPool,
		VkCommandPoolResetFlags						flags);

	void QueueSubmit(
		VkQueue										queue,
		uint32_t									submitCount,
		const VkSubmitInfo*							pSubmits,
		VkFence										fence						= VK_NULL_HANDLE);




	/*Memory management*/
	void AllocateMemory(
		VkDevice                                    device,
		VkDeviceSize								allocationSize,
		uint32_t									memoryTypeIndex,
		VkDeviceMemory*                             pMemory,
		void*									pNext						= nullptr,
		const VkAllocationCallbacks*                pAllocator					= nullptr);

	void MapMemory(
		VkDevice                                    device,
		VkDeviceMemory                              memory,
		void**                                      ppData,
		VkDeviceSize                                size						= VK_WHOLE_SIZE,
		VkDeviceSize                                offset						= 0,
		VkMemoryMapFlags                            flags						= 0);




	/*Various operations*/
	void CopyDataBetweenBuffers(
		VkCommandBuffer								cmdBuffer,
		VkBuffer srcBuffer, VkDeviceSize srcOffset,
		VkBuffer dstBuffer, VkDeviceSize dstOffset,
		VkDeviceSize								size);




	/*Utilities*/
	uint32_t ChooseHeapFromFlags(
		VkPhysicalDevice							physicalDevice,
		const VkMemoryRequirements*					memoryRequirements,
		VkMemoryPropertyFlags						requiredFlags,
		VkMemoryPropertyFlags						preferredFlags);




	/*Enumerations and get functions*/
	VkSubresourceLayout GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource);
	std::vector<VkPhysicalDevice> EnumeratePhysicalDevices(VkInstance instance);
	VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice);
	VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties(VkPhysicalDevice phydev);
	VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures(VkPhysicalDevice phydev);
	std::vector<VkPhysicalDeviceFeatures> EnumeratePhysicalDeviceFeatures(VkInstance instance);
	std::vector<VkQueueFamilyProperties> EnumeratePhysicalDeviceQueueFamilyProperties(VkPhysicalDevice phydev);
	std::vector<std::vector<VkQueueFamilyProperties>> EnumeratePhysicalDeviceQueueFamilyProperties(VkInstance instance);
	VkFormatProperties GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format);
	VkImageFormatProperties GetPhysicalDeviceImageFormatProperties(
		VkPhysicalDevice							physicalDevice,
		VkFormat									format,
		VkImageType									type,
		VkImageTiling								tiling,
		VkImageUsageFlags							usage,
		VkImageCreateFlags							flags);


	/*Property printing*/
	void PrintPhysicalDeviceProperties( VkPhysicalDeviceProperties prop);
	void PrintQueueFamilyProperties(VkQueueFamilyProperties fam);
	void PrintPhysicalDeviceMemoryProperties(VkPhysicalDeviceMemoryProperties prop);


};

