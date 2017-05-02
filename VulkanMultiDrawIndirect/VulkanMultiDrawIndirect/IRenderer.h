#pragma once
#pragma once
#include "VulkanHelpers.h"
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

class IRenderer
{

public:
	typedef uint32_t MeshHandle;
	typedef uint32_t TextureHandle;
	typedef uint32_t TranslationHandle;
	typedef uint32_t BoundingHandle;

public:

	virtual void Render(void) = 0;


	virtual MeshHandle CreateMesh(const std::string& file) = 0;
	virtual uint32_t  CreateTexture(const char* path) = 0;
	virtual TranslationHandle CreateTranslation(const DirectX::XMMATRIX& translation) = 0;
	virtual void Submit(MeshHandle mesh, TextureHandle texture, TranslationHandle translation) = 0;

	virtual void UpdateTranslation(const DirectX::XMMATRIX& translation, TranslationHandle translationHandle) = 0;

	virtual void SetViewMatrix(const DirectX::XMMATRIX& view) = 0;
	virtual void SetProjectionMatrix(const DirectX::XMMATRIX& projection) = 0;


	virtual float GetAspect() = 0;

	//These two shouldnt be here but I cba to refactor the vulkan renderer
	virtual void FrustumCull(VkCommandBuffer& buffer, uint32_t start, uint32_t count) const = 0;
	virtual void RecordDrawCalls(VkCommandBuffer& buffer, uint32_t start, uint32_t count) const = 0;

public:
	virtual int StartTest() = 0;
	virtual float EndTest() = 0;





};
