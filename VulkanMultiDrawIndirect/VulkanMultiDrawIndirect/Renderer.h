#pragma once
#include "VulkanHelpers.h"
#include <vector>
#include <SDL.h>
#include <SDL_syswm.h>

#pragma comment(lib, "vulkan-1.lib")

class Renderer
{
public:
	Renderer(HWND hwnd, uint32_t width, uint32_t height);
	~Renderer();
	void Render(void);


	const void/*Mesh**/ CreateMesh(/*MeshData*/);
	const void/*Texture2D*/  CreateTexture(const char* path);
	const void Submit(/*Mesh*/);
	const void Unsubmit(/*Mesh*/);

private:
	const void _CreateSurface(HWND hwnd);
	const void _CreateSwapChain();
private:
	uint32_t _width;
	uint32_t _height;


	VkInstance _instance;
	std::vector<VkPhysicalDevice> _devices;
	VkDevice _device;
	VkCommandPool _cmdPool;
	VkCommandBuffer _cmdBuffer;
	VkQueue _queue;
	VkDebugReportCallbackEXT _debugCallback;
	VkSurfaceKHR _surface;
	VkFormat _swapchainFormat;
	VkExtent2D _swapchainExtent;
	VkSwapchainKHR _swapchain;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

};