#pragma once
#include "VulkanHelpers.h"
#include "IRenderer.h"
#include <vector>
#include <Parsers.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <unordered_map>
#include "GPUTimer.h"
#include "Texture2D.h"
#include "VertexBufferHandler.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <fstream>
#include "CPUTimer.h"
#include <functional>

#pragma comment(lib, "vulkan-1.lib")

class Renderer : public IRenderer
{
	struct PushConstants
	{
		uint32_t PositionOffset;
		uint32_t TexcoordOffset;
		uint32_t NormalOffset;
		uint32_t Translation;
		uint32_t Texture;
	};

public:
	Renderer(HWND hwnd, uint32_t width, uint32_t height);
	~Renderer();
	void Render(void);


	MeshHandle CreateMesh(const std::string& file);
	uint32_t  CreateTexture(const char* path);
	TranslationHandle CreateTranslation(const DirectX::XMMATRIX& translation);
	void Submit(MeshHandle mesh, TextureHandle texture, TranslationHandle translation);
	//void Unsubmit(/*Mesh*/);

	void UpdateTranslation(const DirectX::XMMATRIX& translation, TranslationHandle translationHandle);

	void SetViewMatrix(const DirectX::XMMATRIX& view);
	void SetProjectionMatrix(const DirectX::XMMATRIX& projection);


	float GetAspect() { return (float)_width / _height; }


	void FrustumCull(VkCommandBuffer& buffer, uint32_t start, uint32_t count) const;
	void RecordDrawCalls(VkCommandBuffer& buffer, uint32_t start, uint32_t count) const;
private:
	typedef void(Renderer::*RenderStrategyFP)();

	void _UpdateViewProjection();

	RenderStrategyFP _currentRenderStrategy;



	void _RenderTraditionalRecord(void);
	void _RenderTraditionalResubmit(void);
	void _RecordTraditionalCmdBuffer(VkCommandBuffer& cmdBuf, bool rerecord);
	void _RenderIndirectRecord(void);
	void _RenderIndirectResubmit(void);
	void _RecordIndirectCmdBuffer(VkCommandBuffer& cmdBuf, bool rerecord);
	void _RecordCmdBuffer(VkCommandBuffer& cmdBuf, bool rerecord, std::function<void(VkRenderPassBeginInfo& beginInfo, VkViewport& viewport, VkRect2D& scissor)> makeRenderPass);

	void _SubmitCmdBuffer(VkCommandBuffer& cmdBuf, VkQueue& queue);

	void _BlitSwapchain(void);
	void _CreateSurface(HWND hwnd);
	void _CreateSwapChain();
	void _CreateSemaphores(void);
	bool _AllocateMemory(VkMemoryPropertyFlagBits desiredProps, const VkMemoryRequirements& memReq, VkDeviceMemory& memory);
	void _CreateOffscreenImage(void);
	void _CreateOffscreenImageView(void);
	void _CreateDepthBufferImage(void);
	void _CreateDepthBufferImageView(void);
	void _CreateRenderPass(void);
	void _CreateFramebuffer(void);
	void _CreateShaders(void);
	void _CreateShader(const char* shaderCode, VkShaderModule& shader);
	void _CreateVPUniformBuffer();
	void _CreateSampler();

	struct VPUniformBuffer
	{
		DirectX::XMFLOAT4X4 view; //Identity matrix as default.
		DirectX::XMFLOAT4X4 projection;
		
		DirectX::XMFLOAT4 frustumOrientation;	
		DirectX::XMFLOAT4 furstumOrigin;

		float RightSlope;           // Positive X slope (X/Z).
		float LeftSlope;            // Negative X slope.
		float TopSlope;             // Positive Y slope (Y/Z).
		float BottomSlope;          // Negative Y slope.
		
		float Near, Far;            // Z of the near plane and far plane.


	};
	void _CreatePipelineLayout(void);
	void _CreatePipeline(void);
	void _CreateDescriptorStuff();


public:
	int StartTest();
	float EndTest();

private:

	CPUTimer _timer;
	
	uint32_t _frameCount;
	float _frameTimes;
	bool _testRunning;

	uint32_t _width;
	uint32_t _height;
	GPUTimer* _gpuTimer;

	VkInstance _instance;
	std::vector<VkPhysicalDevice> _devices;
	VkDevice _device;
	VkCommandPool _mostlyDynamicCmdPool;
	VkCommandPool _mostlyStaticCmdPool;
	VkCommandBuffer _cmdBuffer;
	VkCommandBuffer _traditionalCmdB;
	VkCommandBuffer _blitCmdBuffer;
	VkCommandBuffer _indirectResubmitCmdBuf;

	static const uint8_t NUM_SEC_BUFFERS = 8;
	VkCommandPool _secCmdPools[NUM_SEC_BUFFERS];
	VkCommandBuffer _secBuffers[NUM_SEC_BUFFERS];

	VkQueue _queue;
	VkDebugReportCallbackEXT _debugCallback;
	VkSurfaceKHR _surface;
	VkFormat _swapchainFormat;
	VkExtent2D _swapchainExtent;
	VkSwapchainKHR _swapchain;
	VkSampler _sampler;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	//std::unordered_map<std::string, Texture2D*> _textures;
	//Buffer for the view and projection matrix.
	VPUniformBuffer _ViewProjection;
	VkBuffer _VPUniformBuffer;
	VkDeviceMemory _VPUniformBufferMemory;
	VkBuffer _VPUniformBufferStaging;//Used for updating the uniform buffer
	VkDeviceMemory _VPUniformBufferMemoryStaging;


	bool _doCulling;
	DirectX::BoundingFrustum _frustum;
	DirectX::BoundingFrustum _frustumTransformed;

	bool _doThreadedRecord;

	std::vector<DirectX::XMFLOAT4X4> _translations;

	std::vector <std::tuple<uint32_t, uint32_t>> _translationOffsets;
	std::vector<std::tuple<uint32_t, uint32_t, uint32_t, ArfData::Data, DirectX::BoundingBox>> _meshes;
	std::vector<std::tuple<MeshHandle, TextureHandle, TranslationHandle>> _renderMeshes; // The actual meshes to render during a frame
	std::vector<std::tuple<MeshHandle, TextureHandle, TranslationHandle>> _submitedObjects; // The actual meshes to render during a frame

	VkDescriptorPool _descPool;
	VkDescriptorSetLayout _descLayout;
	VkDescriptorSet _descSet;

	std::unordered_map<std::string, uint32_t> _StringToTextureHandle;
	std::vector<Texture2D> _textures;

	VkSemaphore _imageAvailable = VK_NULL_HANDLE;
	VkSemaphore _swapchainBlitComplete = VK_NULL_HANDLE;
	VkImage _offscreenImage = VK_NULL_HANDLE;
	VkDeviceMemory _offscreenImageMemory = VK_NULL_HANDLE;
	VkImageView _offscreenImageView = VK_NULL_HANDLE;
	VkImage _depthBufferImage = VK_NULL_HANDLE;
	VkDeviceMemory _depthBufferImageMemory = VK_NULL_HANDLE;
	VkImageView _depthBufferImageView = VK_NULL_HANDLE;
	VkRenderPass _renderPass = VK_NULL_HANDLE;
	VkFramebuffer _framebuffer = VK_NULL_HANDLE;
	VkShaderModule _vertexShader = VK_NULL_HANDLE;
	VkShaderModule _fragmentShader = VK_NULL_HANDLE;
	VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
	VkPipeline _pipeline = VK_NULL_HANDLE;
	VkPipeline _indirectPipeline = VK_NULL_HANDLE;

	VertexBufferHandler* _vertexBufferHandler;



};
