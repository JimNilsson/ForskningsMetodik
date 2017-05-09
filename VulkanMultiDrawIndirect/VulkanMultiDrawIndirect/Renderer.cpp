#include "Renderer.h"
#undef min
#undef max
#include <array>
#include <algorithm>
#include <fstream>
#include <Parsers.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <ConsoleThread.h>


using namespace DirectX;
using namespace std;


void FrustumCullingThread(VkCommandBuffer* buffer, const Renderer* renderer, uint32_t start, uint32_t count)
{
	renderer->FrustumCull(*buffer, start, count);
}

void RecordDrawCallsThread(VkCommandBuffer* buffer, const Renderer* renderer, uint32_t start, uint32_t count)
{
	renderer->RecordDrawCalls(*buffer, start, count);
}





Renderer::Renderer(HWND hwnd, uint32_t width, uint32_t height) :_width(width), _height(height), _currentRenderStrategy(&Renderer::_RenderTraditionalResubmit), _doThreadedRecord(true), _doCulling(true), _testRunning(false)
{

	/************Create Instance*************/
	const std::vector<const char*> validationLayers = {
		//"VK_LAYER_LUNARG_standard_validation"
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

	for (uint32_t i = 0; i < queueFamInfo[0].size(); i++)
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
	vector<const char*> deviceExtensions = { "VK_KHR_swapchain", "VK_KHR_shader_draw_parameters" };
	VkPhysicalDeviceFeatures vpdf = {};
	vpdf.shaderStorageImageExtendedFormats = VK_TRUE;
	vpdf.samplerAnisotropy = VK_TRUE;
	vpdf.multiDrawIndirect = VK_TRUE;
	auto lInfo = VulkanHelpers::MakeDeviceCreateInfo(1, &queueInfo, 0, nullptr, &vpdf, nullptr, deviceExtensions.size(), deviceExtensions.data());
	VulkanHelpers::CreateLogicDevice(_devices[0], &lInfo, &_device);

	// Get the queue
	vkGetDeviceQueue(_device, queueIndex, 0, &_queue);
	
	// Create command pool
	auto cmdPoolInfo = VulkanHelpers::MakeCommandPoolCreateInfo(queueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	VulkanHelpers::CreateCommandPool(_device, &cmdPoolInfo, &_mostlyDynamicCmdPool);
	
	for (uint8_t i = 0; i < NUM_SEC_BUFFERS; i++)
	{
		VulkanHelpers::CreateCommandPool(_device, &cmdPoolInfo, &_secCmdPools[i]);
	}


	cmdPoolInfo = VulkanHelpers::MakeCommandPoolCreateInfo(queueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VulkanHelpers::CreateCommandPool(_device, &cmdPoolInfo, &_mostlyStaticCmdPool);

	
	


	// Allocate cmd buffer
	VulkanHelpers::AllocateCommandBuffers(_device, &_cmdBuffer, _mostlyDynamicCmdPool);

	
	VulkanHelpers::AllocateCommandBuffers(_device, &_blitCmdBuffer, _mostlyDynamicCmdPool);
	VulkanHelpers::AllocateCommandBuffers(_device, &_traditionalCmdB, _mostlyStaticCmdPool);
	VulkanHelpers::AllocateCommandBuffers(_device, &_indirectResubmitCmdBuf, _mostlyStaticCmdPool);


	for (uint8_t i = 0; i < NUM_SEC_BUFFERS; i++)
	{
		VulkanHelpers::AllocateCommandBuffers(_device, &_secBuffers[i], _secCmdPools[i], VK_COMMAND_BUFFER_LEVEL_SECONDARY, 1);
	}
	


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


	_CreateVPUniformBuffer();
	_CreateSampler();
	_CreateDescriptorStuff();
	_CreateShaders();
	_CreatePipelineLayout();
	_CreatePipeline();


/*

	struct P
	{
		float x, y, z, w;
	};

	P triangle[] = {
		{-0.5f, 0.5f, 0.0f, 1.0f},
		{0.0f, -0.5f, 0.0f, 1.0f },
		{0.5f, 0.5f, 0.0f, 1.0f }
	};


	_vertexBufferHandler->CreateBuffer(triangle, 3, VertexType::Position);*/



	DebugUtils::DebugConsole::Command_Structure renderStrategyCmd =
	{
		this,
		[](void* userData, int argc, char** argv) {
		if (argc < 2)
			return;
		Renderer* r = static_cast<Renderer*>(userData);

		char* opt;
		if (DebugUtils::GetArg("-c", &opt, argc, argv))
		{
			if (std::string("on") == opt)
			{
				r->_doCulling = true;
			}
			else if (std::string("off") == opt)
			{
				r->_doCulling = false;
			}
			else
			{
				printf("\n -c [on/off]\t\t Render with frustum culling.\n");
			}
			if (argc < 4)
			{
				return;
			}
		}

		if (DebugUtils::GetArg("-m", &opt, argc, argv))
		{
			if (std::string("on") == opt)
			{
				r->_doThreadedRecord = true;
			}
			else if (std::string("off") == opt)
			{
				r->_doThreadedRecord = false;
			}
			else
			{
				printf("\n -m [on/off]\t\t Record draw commands with multithreading.\n");
			}
			if (argc < 4)
			{
				return;
			}
		}

		if (DebugUtils::GetArg("-i", nullptr, argc, argv))
		{
			if (DebugUtils::GetArg("-r", nullptr, argc, argv)) // Indirect Record
			{
				r->_currentRenderStrategy = &Renderer::_RenderIndirectRecord;
				return;
			}
			else if (DebugUtils::GetArg("-s", nullptr, argc, argv)) // Indirect Resubmit
			{
				r->_RecordIndirectCmdBuffer(r->_indirectResubmitCmdBuf, false);
				r->_currentRenderStrategy = &Renderer::_RenderIndirectResubmit;
				return;
			}
		}
		else if (DebugUtils::GetArg("-t", nullptr, argc, argv))
		{
			if (DebugUtils::GetArg("-r", nullptr, argc, argv)) // Traditional
			{
				r->_currentRenderStrategy = &Renderer::_RenderTraditionalRecord;
				return;
			}
			else if (DebugUtils::GetArg("-s", nullptr, argc, argv)) // Traditional resubmit
			{
				r->_RecordTraditionalCmdBuffer(r->_traditionalCmdB, false);

				r->_currentRenderStrategy = &Renderer::_RenderTraditionalResubmit;
				return;
			}
		}

		printf("Usage: strategy OPTION\nSets rendering strategy.\n\n");
		printf("*** Render Types ***\n");
		printf("\t -i\t\t Use multidraw indirect rendering\n");
		printf("\t -t\t\t Use traditional rendering\n");

		printf("\n\n*** Recording options ***\n");
		printf("\t -r\t\t Record the command buffer each frame.\n");
		printf("\t -s\t\t Resubmit a pre-recorded command buffer.\n");	
		printf("\t -c [on/off]\t\t Render with frustum culling.\n");
	},
		[](void* userData, int argc, char** argv) {
		printf("Usage: strategy OPTION\nSets rendering strategy.\n\n");
		printf("*** Render Types ***\n");
		printf("\t -i\t\t Use multidraw indirect rendering\n");
		printf("\t -t\t\t Use traditional rendering\n");

		printf("\n\n*** Recording options ***\n");
		printf("\t -r\t\t Record the command buffer each frame.\n");
		printf("\t -s\t\t Resubmit a pre-recorded command buffer.\n");
		printf("\t -c [on/off]\t\t Render with frustum culling.\n");
		},
		"strat",
		"Sets the rendering strategy."
	};

	DebugUtils::ConsoleThread::AddCommand(&renderStrategyCmd);



}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(_device);


	vkDestroyDescriptorSetLayout(_device, _descLayout, nullptr);

	vkDestroyDescriptorPool(_device, _descPool, nullptr);
	delete _vertexBufferHandler;
	delete _gpuTimer;
	vkDestroyPipeline(_device, _indirectPipeline, nullptr);
	vkDestroyPipeline(_device, _pipeline, nullptr);
	vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
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
	vkDestroySampler(_device, _sampler, nullptr);
	vkFreeMemory(_device, _offscreenImageMemory, nullptr);
	vkDestroyImage(_device, _offscreenImage, nullptr);
	for (uint8_t i = 0; i < NUM_SEC_BUFFERS; i++)
	{
		vkDestroyCommandPool(_device, _secCmdPools[i], nullptr);
	}
	vkDestroyCommandPool(_device, _mostlyDynamicCmdPool, nullptr);
	vkDestroyCommandPool(_device, _mostlyStaticCmdPool, nullptr);
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
int Renderer::StartTest()
{
	
	_frameCount = 0;
	_frameTimes = 0.0f;
	_testRunning = true;
	return 0;
}

float Renderer::EndTest()
{

	float avgTime = _frameTimes / _frameCount;

	_frameCount = 0;
	_testRunning = false;
	return avgTime;
}

void Renderer::Render(void)
{
	_timer.TimeStart("Frame");
	vkQueueWaitIdle(_queue);

	//printf("GPU Time: %f\n", _gpuTimer->GetTime(0));

	// Begin rendering stuff while we potentially wait for swapchain image

	// Flush the translations on the host to the gpu
	_vertexBufferHandler->FlushBuffer(VertexType::Translation);

	(*this.*_currentRenderStrategy)();

//	_SubmitCmdBuffer(_cmdBuffer, _queue);

	//printf("Finished in: %f", _gpuTimer->GetTime(0));






	// While the scene is rendering we can get the swapchain image and begin
	// transitioning it. When it's time to blit we must synchronize to make
	// sure that the image is finished for us to read. 
	_BlitSwapchain();

	_timer.TimeEnd("Frame");
	if (_testRunning)
	{
		_frameTimes += _timer.GetTime("Frame");
		_frameCount++;
	}
	
}

Renderer::MeshHandle Renderer::CreateMesh(const std::string & file)
{
	ArfData::Data data;
	ArfData::DataPointers dataPointers;
	if (ParseObj(file.c_str(), &data, &dataPointers) != 0)
	{
		throw runtime_error("Failed to load mesh from file");
	}

	uint32_t bufferCount = data.NumFace * 3;
	size_t index = 0;
	struct P
	{
		float x, y, z, w;
	};
	auto posBuffer = new P[bufferCount];
	auto texBuffer = new ArfData::TexCoord[bufferCount];
	auto normBuffer = new P[bufferCount];
	for (uint8_t subM = 0; subM < data.NumSubMesh; subM++)
	{
		auto& subMesh = dataPointers.subMesh[subM];
		for (auto f = subMesh.faceStart; f < subMesh.faceCount + subMesh.faceStart; f++)
		{
			auto& face = dataPointers.faces[f];
			for (uint8_t fi = 0; fi < face.indexCount; fi++)
			{
				auto& faceIndex = face.indices[fi];
				if (faceIndex.index[POSITION_INDEX] != INDEX_NULL)
				{
					memcpy(&posBuffer[index], &dataPointers.positions[faceIndex.index[POSITION_INDEX]], sizeof(ArfData::Position));
					posBuffer[index].w = 1.0f;
				}
				if (faceIndex.index[TEXCOORD_INDEX] != INDEX_NULL)
				{
					ArfData::TexCoord tc = dataPointers.texCoords[faceIndex.index[TEXCOORD_INDEX]];
					tc.v = 1.0f - tc.v;
					memcpy(&texBuffer[index], &tc, sizeof(tc));
				}
				if (faceIndex.index[NORMAL_INDEX] != INDEX_NULL)
				{
					memcpy(&normBuffer[index], &dataPointers.normals[faceIndex.index[NORMAL_INDEX]], sizeof(ArfData::Normal));
					normBuffer[index].w = 0.0f;
				}
				index++;			
			}
		}
	}



	uint32_t positionOffset = _vertexBufferHandler->CreateBuffer(posBuffer, bufferCount, VertexType::Position);
	uint32_t texcoordOffset = _vertexBufferHandler->CreateBuffer(texBuffer, bufferCount, VertexType::TexCoord);
	uint32_t normalOffset = _vertexBufferHandler->CreateBuffer(normBuffer, bufferCount, VertexType::Normal);


	BoundingBox b;
	BoundingBox::CreateFromPoints(b, (size_t)bufferCount, (XMFLOAT3*)posBuffer, sizeof(P));

	delete[] dataPointers.buffer;
	delete[] posBuffer;
	delete[] texBuffer;
	delete[] normBuffer;
	dataPointers.buffer = nullptr;

	uint32_t meshIndex = _meshes.size();
	_meshes.push_back({ positionOffset, texcoordOffset, normalOffset, data , b});

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
	cmdBufAllInf.commandPool = _mostlyDynamicCmdPool;
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
	vkFreeCommandBuffers(_device, _mostlyDynamicCmdPool, 1, &oneTimeBuffer);


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


	VkDescriptorImageInfo vkdii;
	vkdii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkdii.imageView = _textures.back()._imageView;
	vkdii.sampler = VK_NULL_HANDLE;
	if (_textures.size() == 1)
	{
		std::vector<VkWriteDescriptorSet> wds;
		for (uint32_t i = 0; i < 2; i++)
		{
			wds.push_back(VulkanHelpers::MakeWriteDescriptorSet(_descSet, 0, i, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &vkdii, nullptr, nullptr));
		}
		
		vkUpdateDescriptorSets(_device, wds.size(), wds.data(), 0, nullptr);
	}
	else
	{
		auto wds = VulkanHelpers::MakeWriteDescriptorSet(_descSet, 0, _textures.size() - 1, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &vkdii, nullptr, nullptr);

		vkUpdateDescriptorSets(_device, 1, &wds, 0, nullptr);
	}
	

	return _textures.size() - 1;
}
using namespace DirectX;
Renderer::TranslationHandle Renderer::CreateTranslation(const XMMATRIX & translation)
{
	XMFLOAT4X4 temp;
	XMStoreFloat4x4(&temp, translation);
	XMMATRIX columnMajor = XMMatrixTranspose(translation);
	uint32_t offset = _vertexBufferHandler->CreateBuffer((void*)(&columnMajor), 1, VertexType::Translation);
	_translations.push_back(temp);
	_translationOffsets.push_back({ offset, _translations.size() - 1 });
	auto translationHandle = _translationOffsets.size() - 1;
	return translationHandle;
}

void Renderer::Submit(MeshHandle mesh, TextureHandle texture, TranslationHandle translation)
{
	_renderMeshes.push_back({ mesh, texture, translation });

	PushConstants pushConstants;
	pushConstants.PositionOffset = get<0>(_meshes[mesh]);
	pushConstants.TexcoordOffset = get<1>(_meshes[mesh]);
	pushConstants.NormalOffset = get<2>(_meshes[mesh]);
	pushConstants.Translation = get<0>(_translationOffsets[translation]);
	pushConstants.Texture = texture;
	_vertexBufferHandler->CreateBuffer(&pushConstants, 8, VertexType::Index);

	VkDrawIndirectCommand s = {};
	s.vertexCount = get<3>(_meshes[mesh]).NumFace * 3;
	s.instanceCount = 1;
	_vertexBufferHandler->CreateBuffer(&s, 1, VertexType::IndirectBuffer);
}

void Renderer::UpdateTranslation(const DirectX::XMMATRIX & translation, TranslationHandle translationHandle)
{
	XMStoreFloat4x4(&_translations[get<1>(_translationOffsets[translationHandle])], translation);
	_vertexBufferHandler->Update((void*)&XMMatrixTranspose(translation), 1, VertexType::Translation, get<0>(_translationOffsets[translationHandle]));
}

void Renderer::SetViewMatrix(const XMMATRIX & view)
{
	XMStoreFloat4x4(&_ViewProjection.view, XMMatrixTranspose(view));
	_frustum.Transform(_frustumTransformed,XMMatrixInverse(nullptr, view));

	memcpy(&_ViewProjection.furstumOrigin, &_frustumTransformed.Origin, sizeof(XMFLOAT3));
	_ViewProjection.furstumOrigin.w = 0.0f;
	_ViewProjection.frustumOrientation = _frustumTransformed.Orientation;
	_ViewProjection.BottomSlope = _frustumTransformed.BottomSlope;
	_ViewProjection.Far = _frustumTransformed.Far;
	_ViewProjection.LeftSlope = _frustumTransformed.LeftSlope;
	_ViewProjection.Near = _frustumTransformed.Near;
	_ViewProjection.RightSlope = _frustumTransformed.RightSlope;
	_ViewProjection.TopSlope = _frustumTransformed.TopSlope;

	_UpdateViewProjection();
}

void Renderer::SetProjectionMatrix(const XMMATRIX & projection)
{
	XMStoreFloat4x4(&_ViewProjection.projection, XMMatrixTranspose(DirectX::XMMatrixScaling(1.0f, -1.0f, 1.0f) * projection));

	BoundingFrustum::CreateFromMatrix(_frustum, projection);

	_UpdateViewProjection();
}

void Renderer::FrustumCull(VkCommandBuffer & buffer, uint32_t start, uint32_t count)const
{
	BoundingOrientedBox bo;

	for (auto i = start; i < start + count; i++)
	{
		auto& meshHandle = get<0>(_renderMeshes[i]);
		auto& translationHandle = get<2>(_renderMeshes[i]);

		BoundingOrientedBox::CreateFromBoundingBox(bo, get<4>(_meshes[meshHandle]));
		auto& world = XMLoadFloat4x4(&_translations[get<1>(_translationOffsets[get<2>(_renderMeshes[i])])]);
		bo.Transform(bo, world);
		
		if (_frustumTransformed.Intersects(bo))
		{
			const ArfData::Data& meshData = get<3>(_meshes[meshHandle]);
			vkCmdDraw(buffer, meshData.NumFace * 3, 1, 0, i);
		}

	
	}
	return void();
}

void Renderer::RecordDrawCalls(VkCommandBuffer & buffer, uint32_t start, uint32_t count) const
{
	for (auto i = start; i < start + count; i++)
	{
		auto& meshHandle = get<0>(_renderMeshes[i]);
		const ArfData::Data& meshData = get<3>(_meshes[meshHandle]);
		vkCmdDraw(buffer, meshData.NumFace * 3, 1, 0, i);
	}

}

void Renderer::_UpdateViewProjection()
{
	void* src;
	VulkanHelpers::MapMemory(_device, _VPUniformBufferMemoryStaging, &src, sizeof(VPUniformBuffer));
	memcpy(src, &_ViewProjection, sizeof(VPUniformBuffer));
	vkUnmapMemory(_device, _VPUniformBufferMemoryStaging);

	//TODO: Make a fence instead of waiting for idle.
	vkQueueWaitIdle(_queue);
	VulkanHelpers::BeginCommandBuffer(_cmdBuffer);
	VulkanHelpers::CopyDataBetweenBuffers(_cmdBuffer, _VPUniformBufferStaging, 0, _VPUniformBuffer, 0, sizeof(VPUniformBuffer));
	vkEndCommandBuffer(_cmdBuffer);
	auto& sInfo = VulkanHelpers::MakeSubmitInfo(1, &_cmdBuffer);
	VulkanHelpers::QueueSubmit(_queue, 1, &sInfo);
	vkQueueWaitIdle(_queue);
}

// Render the scene in a traditional manner, i.e. rerecord the draw calls to
// work with a dynamic scene.
void Renderer::_RenderTraditionalRecord()
{
	_RecordTraditionalCmdBuffer(_cmdBuffer, true);

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

void Renderer::_RenderTraditionalResubmit()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_traditionalCmdB;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void Renderer::_RecordTraditionalCmdBuffer(VkCommandBuffer& cmdBuf, bool rerecord)
{
	auto makeRenderPass = [this, cmdBuf, rerecord](VkRenderPassBeginInfo& beginInfo, VkViewport& viewport, VkRect2D& scissor)
	{
		if ((_doCulling || _doThreadedRecord) && rerecord)
		{
			vkCmdBeginRenderPass(cmdBuf, &beginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

			std::thread threads[NUM_SEC_BUFFERS];

			VkCommandBufferInheritanceInfo ini = {};
			ini.renderPass = _renderPass;
			ini.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			ini.framebuffer = _framebuffer;

			auto meshesPerThread = _renderMeshes.size() / NUM_SEC_BUFFERS;

			for (int i = 0; i < NUM_SEC_BUFFERS; i++)
			{
				VkCommandBufferUsageFlags usageFlags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				VulkanHelpers::BeginCommandBuffer(_secBuffers[i], usageFlags, &ini);
				vkCmdBindPipeline(_secBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
				vkCmdSetViewport(_secBuffers[i], 0, 1, &viewport);
				vkCmdSetScissor(_secBuffers[i], 0, 1, &scissor);

				vkCmdBindDescriptorSets(_secBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descSet, 0, nullptr);

				if(_doCulling)
					threads[i] = std::thread(FrustumCullingThread, &_secBuffers[i], this, meshesPerThread * i, meshesPerThread + (i == NUM_SEC_BUFFERS - 1 ? _renderMeshes.size() % meshesPerThread : 0));
				else
					threads[i] = std::thread(RecordDrawCallsThread, &_secBuffers[i], this, meshesPerThread * i, meshesPerThread + (i == NUM_SEC_BUFFERS - 1 ? _renderMeshes.size() % meshesPerThread : 0));
			}
			for (int i = 0; i < NUM_SEC_BUFFERS; i++)
			{
				threads[i].join();
				vkEndCommandBuffer(_secBuffers[i]);
			}

			vkCmdExecuteCommands(cmdBuf, NUM_SEC_BUFFERS, _secBuffers);

			vkCmdEndRenderPass(cmdBuf);
		}
		else
		{
			vkCmdBeginRenderPass(cmdBuf, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);


			vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
			vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
			vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descSet, 0, nullptr);

			uint32_t firstInstance = 0; // This is used to generate offsets for the shader similarly to DrawID for indirect call
			for (auto& mesh : _renderMeshes)
			{
				auto& meshHandle = get<0>(mesh);

				const ArfData::Data& meshData = get<3>(_meshes[meshHandle]);
				vkCmdDraw(cmdBuf, meshData.NumFace * 3, 1, 0, firstInstance);

				firstInstance++;
			}

			vkCmdEndRenderPass(cmdBuf);
		}
	};

	_RecordCmdBuffer(cmdBuf, rerecord, makeRenderPass);
}

void Renderer::_RenderIndirectRecord()
{
	//_vertexBufferHandler->FlushBuffer(VertexType::Translation);
	_vertexBufferHandler->FlushBuffer(VertexType::IndirectBuffer);

	_RecordIndirectCmdBuffer(_cmdBuffer, true);

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

void Renderer::_RenderIndirectResubmit()
{
	//_vertexBufferHandler->FlushBuffer(VertexType::Translation);
	_vertexBufferHandler->FlushBuffer(VertexType::IndirectBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_indirectResubmitCmdBuf;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void Renderer::_RecordIndirectCmdBuffer(VkCommandBuffer& cmdBuf, bool rerecord)
{
	auto makeRenderPass = [this, cmdBuf](VkRenderPassBeginInfo& beginInfo, VkViewport& viewport, VkRect2D& scissor)
	{
		vkCmdBeginRenderPass(cmdBuf, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _indirectPipeline);
		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

		vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descSet, 0, nullptr);

		vkCmdDrawIndirect(cmdBuf, _vertexBufferHandler->GetBuffer(VertexType::IndirectBuffer), 0, _renderMeshes.size(), sizeof(VkDrawIndirectCommand));

		vkCmdEndRenderPass(cmdBuf);
	};

	_RecordCmdBuffer(cmdBuf, rerecord, makeRenderPass);
}

void Renderer::_RecordCmdBuffer(VkCommandBuffer& cmdBuf, bool rerecord, function<void(VkRenderPassBeginInfo& beginInfo, VkViewport& viewport, VkRect2D& scissor)> makeRenderPass)
{
	VkCommandBufferBeginInfo commandBufBeginInfo = {};
	commandBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufBeginInfo.pNext = nullptr;
	commandBufBeginInfo.flags = rerecord ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
	commandBufBeginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(cmdBuf, &commandBufBeginInfo);

	_gpuTimer->Start(cmdBuf, 0);

	// Do the actual rendering

	array<VkClearValue, 2> clearValues = {};
	clearValues[0] = { 0.2f, 0.4f, 0.6f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.renderPass = _renderPass;
	beginInfo.framebuffer = _framebuffer;
	beginInfo.renderArea = { 0, 0, _swapchainExtent.width, _swapchainExtent.height };
	beginInfo.clearValueCount = clearValues.size();
	beginInfo.pClearValues = clearValues.data();

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = _swapchainExtent.width;
	viewport.height = _swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = _swapchainExtent;

	makeRenderPass(beginInfo, viewport, scissor);

	// TODO: As of now there is no synchronization point between rendering to
	// the offscreen buffer and using that image as blit source later. At the
	// place of this comment we could probably issue an event that is waited on
	// in the blit buffer before blitting to make sure rendering is complete.
	// Don't forget to reset the event when we have waited on it.

	_gpuTimer->End(cmdBuf, 0);

	vkEndCommandBuffer(cmdBuf);
}

void Renderer::_SubmitCmdBuffer(VkCommandBuffer & cmdBuf, VkQueue & queue)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
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

void Renderer::_CreateSurface(HWND hwnd)
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

void Renderer::_CreateSwapChain()
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
			if (i.format == VK_FORMAT_B8G8R8A8_UNORM && i.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
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

void Renderer::_CreatePipelineLayout(void)
{
	array<VkDescriptorSetLayout, 1> setLayouts = { _descLayout };

	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.flags = 0;
	layoutInfo.setLayoutCount = setLayouts.size();
	layoutInfo.pSetLayouts = setLayouts.data();
	layoutInfo.pushConstantRangeCount = 0;
	layoutInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to create pipeline layout!");
	}
}

void Renderer::_CreatePipeline(void)
{
	struct SpecializationData
	{
		uint32_t IndirectRendering;
	} data;

	data.IndirectRendering = 0;

	VkSpecializationMapEntry indirectRenderingConstant = {};
	indirectRenderingConstant.constantID = 0;
	indirectRenderingConstant.offset = 0;
	indirectRenderingConstant.size = sizeof(uint32_t);

	VkSpecializationInfo specInfo = {};
	specInfo.mapEntryCount = 1;
	specInfo.pMapEntries = &indirectRenderingConstant;
	specInfo.dataSize = sizeof(SpecializationData);
	specInfo.pData = &data;

	array<VkPipelineShaderStageCreateInfo, 2> stages = {};
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].pNext = nullptr;
	stages[0].flags = 0;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = _vertexShader;
	stages[0].pName = "main";
	stages[0].pSpecializationInfo = &specInfo;

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].pNext = nullptr;
	stages[1].flags = 0;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = _fragmentShader;
	stages[1].pName = "main";
	stages[1].pSpecializationInfo = nullptr;

	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.pNext = nullptr;
	vertexInputState.flags = 0;
	vertexInputState.vertexBindingDescriptionCount = 0;
	vertexInputState.pVertexBindingDescriptions = nullptr;
	vertexInputState.vertexAttributeDescriptionCount = 0;
	vertexInputState.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.pNext = nullptr;
	inputAssembly.flags = 0;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr; // Ignored for dynamic
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr; // Ignored for dynamic

	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.pNext = nullptr;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasConstantFactor = 0.0f;
	rasterizationState.depthBiasClamp = 0.0f;
	rasterizationState.depthBiasSlopeFactor = 0.0f;
	rasterizationState.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pNext = nullptr;
	multisampleState.flags = 0;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable = VK_FALSE;
	multisampleState.minSampleShading = 0.0f;
	multisampleState.pSampleMask = nullptr;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthState = {};
	depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthState.pNext = nullptr;
	depthState.flags = 0;
	depthState.depthTestEnable = VK_TRUE;
	depthState.depthWriteEnable = VK_TRUE;
	depthState.depthCompareOp = VK_COMPARE_OP_LESS;
	depthState.depthBoundsTestEnable = VK_FALSE;
	depthState.stencilTestEnable = VK_FALSE;
	depthState.front = {};
	depthState.back = {};
	depthState.minDepthBounds = 0.0f;
	depthState.maxDepthBounds = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.blendEnable = VK_FALSE; // Other parameters unnecessary
	colorBlendAttachment.colorWriteMask = 0xf;

	VkPipelineColorBlendStateCreateInfo blendState = {};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.pNext = nullptr;
	blendState.flags = 0;
	blendState.logicOpEnable = VK_FALSE;
	blendState.logicOp = VK_LOGIC_OP_AND;
	blendState.attachmentCount = 1;
	blendState.pAttachments = &colorBlendAttachment;
	blendState.blendConstants[0] = 0.0f;
	blendState.blendConstants[1] = 0.0f;
	blendState.blendConstants[2] = 0.0f;
	blendState.blendConstants[3] = 0.0f;

	array<VkDynamicState, 2> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = stages.size();
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vertexInputState;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizationState;
	pipelineInfo.pMultisampleState = &multisampleState;
	pipelineInfo.pDepthStencilState = &depthState;
	pipelineInfo.pColorBlendState = &blendState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = _renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	VkResult result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to create pipeline");
	}

	data.IndirectRendering = 1;
	result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_indirectPipeline);
	if (result != VK_SUCCESS)
	{
		throw runtime_error("Failed to create pipeline");
	}
}

void Renderer::_CreateDescriptorStuff()
{
	/* Create the descriptor pool*/
	std::vector<VkDescriptorPoolSize> _poolSizes;
	_poolSizes.push_back(
	{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2 });
	_poolSizes.push_back(
	{ VK_DESCRIPTOR_TYPE_SAMPLER, 1 });
	_poolSizes.push_back(
	{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });
	auto& dps = _vertexBufferHandler->GetDescriptorPoolSizes();
	_poolSizes.insert(_poolSizes.end(), dps.begin(), dps.end());


	VulkanHelpers::CreateDescriptorPool(_device, &_descPool, 0, 10, _poolSizes.size(), _poolSizes.data());



	/* Specify the bindings */
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.push_back({
		(uint32_t)bindings.size(),
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		2,
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
	bindings.push_back({
		(uint32_t)bindings.size(),
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		1,
		VK_SHADER_STAGE_VERTEX_BIT,
		nullptr
	});
	auto& dslb = _vertexBufferHandler->GetDescriptorSetLayoutBindings(3);
	bindings.insert(bindings.end(), dslb.begin(), dslb.end());


	/* Create the descriptor layout. */
	VulkanHelpers::CreateDescriptorSetLayout(_device, &_descLayout, bindings.size(), bindings.data());

	/* Allocate the desciptor set*/
	VulkanHelpers::AllocateDescriptorSets(_device, _descPool, 1, &_descLayout, &_descSet);

	std::vector<VkWriteDescriptorSet> WriteDS;

	_vertexBufferHandler->WriteDescriptorSets(_descSet, 3);
	
	VkDescriptorBufferInfo ubdescInfo;
	ubdescInfo.buffer = _VPUniformBuffer;
	ubdescInfo.offset = 0;
	ubdescInfo.range = VK_WHOLE_SIZE;
	WriteDS.push_back(VulkanHelpers::MakeWriteDescriptorSet(_descSet,2, 0, 1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		nullptr, &ubdescInfo, nullptr));
	
	VkDescriptorImageInfo dii;
	dii.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	dii.imageView = VK_NULL_HANDLE;
	dii.sampler = _sampler;
	WriteDS.push_back(VulkanHelpers::MakeWriteDescriptorSet(_descSet, 1, 0, 1,
		VK_DESCRIPTOR_TYPE_SAMPLER,
		&dii, nullptr, nullptr));

	
	

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

void Renderer::_CreateSampler()
{
	VkSamplerCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext = nullptr;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	info.anisotropyEnable = VK_TRUE;
	info.maxAnisotropy = 16.0;
	info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info.compareEnable = VK_FALSE;
	info.compareOp = VK_COMPARE_OP_ALWAYS;
	info.flags = 0;
	info.maxLod = 0;
	info.minLod = 0;
	info.unnormalizedCoordinates = VK_FALSE;
	info.mipLodBias = 0;

	vkCreateSampler(_device, &info, nullptr, &_sampler);

}


