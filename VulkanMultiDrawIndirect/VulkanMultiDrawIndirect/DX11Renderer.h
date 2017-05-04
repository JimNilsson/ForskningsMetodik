#pragma once

#include "IRenderer.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

#define SAFE_RELEASE(x) {if(x){ x->Release(); x = nullptr;}};

class DX11Renderer : public IRenderer
{
	
private:

	ID3D11Device*                       _device = nullptr;
	ID3D11DeviceContext*                _deviceContext = nullptr;
	IDXGISwapChain*                     _swapChain = nullptr;
	ID3D11RenderTargetView*             _backbufferRTV = nullptr;

	ID3D11InputLayout* _inputLayout;
	ID3D11VertexShader*     _vertexShader;
	ID3D11PixelShader*      _pixelShader;
	ID3D11SamplerState* _samplerState;
	ID3D11RasterizerState* _rasterizerState;

	ID3D11Texture2D* _dsb;
	ID3D11DepthStencilView* _dsv;


	std::vector<ID3D11ShaderResourceView*> _textures; //diffuse maps, normal maps, etc.
	std::vector<ID3D11Buffer*> _vertexBuffers;
	std::vector<ID3D11Buffer*> _indexBuffers;

	uint32_t _width;
	uint32_t _height;
	enum ConstantBuffers
	{
		CB_PER_FRAME,
		CB_PER_OBJECT,
		CB_COUNT
	};

	struct PerFrameBuffer
	{
		DirectX::XMFLOAT4X4 vp;
	};

	struct PerObjectBuffer
	{
		DirectX::XMFLOAT4X4 World;
		DirectX::XMFLOAT4X4 wvInvTrp;
		
	};

	
	struct ViewProj
	{
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 proj;
	};

	struct RenderJob
	{
		uint32_t mesh;
		uint32_t translation;
		uint32_t texture;
	};

	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 nor;
		DirectX::XMFLOAT2 tex;
	};

	

	std::vector<RenderJob> _renderJobs;
	std::vector<DirectX::XMMATRIX> _translations;

	struct MeshStuff
	{
		uint32_t vertexBuffer;
		uint32_t indexBuffer;
		uint32_t vertexCount;
		uint32_t indexCount;
	};
	std::vector<MeshStuff> _meshes;

	ViewProj _vp;

	ID3D11Buffer* _constantBuffers[ConstantBuffers::CB_COUNT] = { nullptr };

public:
	DX11Renderer(HWND hwnd, uint32_t width, uint32_t height);
	~DX11Renderer();


	void Render(void);
	virtual int StartTest();
	virtual float EndTest();

	MeshHandle CreateMesh(const std::string& file);
	uint32_t  CreateTexture(const char* path);
	TranslationHandle CreateTranslation(const DirectX::XMMATRIX& translation);
	void Submit(MeshHandle mesh, TextureHandle texture, TranslationHandle translation);

	void UpdateTranslation(const DirectX::XMMATRIX& translation, TranslationHandle translationHandle);

	void SetViewMatrix(const DirectX::XMMATRIX& view);
	void SetProjectionMatrix(const DirectX::XMMATRIX& projection);
	float GetAspect();

	//These two shouldnt be here but I cba to refactor the vulkan renderer
	void FrustumCull(VkCommandBuffer& buffer, uint32_t start, uint32_t count) const;
	void RecordDrawCalls(VkCommandBuffer& buffer, uint32_t start, uint32_t count) const;

private:
	CPUTimer _timer;
	void _CreateShadersAndInputLayouts();
	void _CreateDepthBuffer();
	void _CreateSamplerState();
	void _CreateViewPort();
	void _CreateRasterizerState();
	void _CreateConstantBuffers();
	bool _testRunning = false;
	float _frameTimes = 0;
	size_t _frameCount = 0;
	int _CreateVertexBuffer(Vertex * vertexData, unsigned vertexCount);
	int	_CreateIndexBuffer(unsigned * indexData, unsigned indexCount);
	

};