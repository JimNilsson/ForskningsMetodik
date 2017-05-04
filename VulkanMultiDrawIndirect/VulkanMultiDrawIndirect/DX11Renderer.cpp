#include "DX11Renderer.h"
#include "DirectXTK\DDSTextureLoader.h"
#include "DirectXTK\WICTextureLoader.h"
#include <exception>
#include <fstream>
#include <sstream>

using namespace DirectX;



DX11Renderer::DX11Renderer(HWND hwnd, uint32_t width, uint32_t height)
{
	_width = width;
	_height = height;
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hwnd;
	scd.SampleDesc.Count = 1;
	scd.Windowed = TRUE;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL, D3D11_SDK_VERSION, &scd, &_swapChain, &_device, NULL, &_deviceContext);
	if (FAILED(hr))
		throw std::exception("Failed to create device and swapchain");

	////Set up GBuffers for deferred shading
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET;


	D3D11_RENDER_TARGET_VIEW_DESC rtvd;
	ZeroMemory(&rtvd, sizeof(rtvd));
	rtvd.Format = td.Format;
	rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvd.Texture2D.MipSlice = 0;


	ID3D11Buffer* backbuffer;
	rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer);
	_device->CreateRenderTargetView(backbuffer, &rtvd, &_backbufferRTV);
	SAFE_RELEASE(backbuffer);

	_CreateShadersAndInputLayouts();
	_CreateDepthBuffer();
	_CreateSamplerState();
	_CreateViewPort();
	_CreateRasterizerState();
	_CreateConstantBuffers();
}

DX11Renderer::~DX11Renderer()
{
	//Dont care lol
	SAFE_RELEASE(_swapChain);
	SAFE_RELEASE(_backbufferRTV);
	SAFE_RELEASE(_inputLayout);
	SAFE_RELEASE(_vertexShader);
	SAFE_RELEASE(_pixelShader);
	SAFE_RELEASE(_samplerState);
	SAFE_RELEASE(_rasterizerState);
	SAFE_RELEASE(_dsb);
	SAFE_RELEASE(_dsv);
	for(auto& t : _textures)
		SAFE_RELEASE(t);
	for(auto& b : _vertexBuffers)
		SAFE_RELEASE(b);
	for (auto& b : _indexBuffers)
		SAFE_RELEASE(b);
	for (auto& b : _constantBuffers)
		SAFE_RELEASE(b);

	SAFE_RELEASE(_deviceContext);
	SAFE_RELEASE(_device);

}

void DX11Renderer::Render(void)
{
	_timer.TimeStart("Frame");
	float clearColor[] = { 0.0f,1.0f,0.0f,0.0f };

	_deviceContext->ClearRenderTargetView(_backbufferRTV, clearColor);
	_deviceContext->ClearDepthStencilView(_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	_deviceContext->IASetInputLayout(_inputLayout);
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceContext->VSSetShader(_vertexShader, nullptr, 0);
	_deviceContext->PSSetShader(_pixelShader, nullptr, 0);
	_deviceContext->OMSetRenderTargets(1, &_backbufferRTV, _dsv);

	PerFrameBuffer pfb;
	XMMATRIX view = XMLoadFloat4x4(&_vp.view);
	XMMATRIX proj = XMLoadFloat4x4(&_vp.proj);
	XMStoreFloat4x4(&pfb.vp, XMMatrixTranspose(view * proj));
	D3D11_MAPPED_SUBRESOURCE mappedSubres;
	_deviceContext->Map(_constantBuffers[ConstantBuffers::CB_PER_FRAME], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubres);
	memcpy(mappedSubres.pData, &pfb, sizeof(pfb));
	_deviceContext->Unmap(_constantBuffers[ConstantBuffers::CB_PER_FRAME], 0);
	_deviceContext->VSSetConstantBuffers(0, 1, &_constantBuffers[ConstantBuffers::CB_PER_FRAME]);
	
	for (auto& j : _renderJobs)
	{
		uint32_t stride = sizeof(Vertex);
		uint32_t offset = 0;
		_deviceContext->IASetVertexBuffers(0, 1, &_vertexBuffers[_meshes[j.mesh].vertexBuffer],&stride, &offset);
		//We dont use the index buffer
		XMMATRIX world = _translations[j.translation];
		PerObjectBuffer pob;
		XMStoreFloat4x4(&pob.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&pob.wvInvTrp, XMMatrixInverse(nullptr, world * view));
		D3D11_MAPPED_SUBRESOURCE msr;
		_deviceContext->Map(_constantBuffers[ConstantBuffers::CB_PER_OBJECT], 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, &pob, sizeof(PerObjectBuffer));
		_deviceContext->Unmap(_constantBuffers[ConstantBuffers::CB_PER_OBJECT], 0);
		_deviceContext->VSSetConstantBuffers(1, 1, &_constantBuffers[ConstantBuffers::CB_PER_OBJECT]);

		_deviceContext->PSSetShaderResources(0, 1, &_textures[j.texture]);

		_deviceContext->Draw(_meshes[j.mesh].vertexCount, 0);

	}

	_swapChain->Present(0, 0);
	_timer.TimeEnd("Frame");
	if (_testRunning)
	{
		_frameTimes += _timer.GetTime("Frame");
		_frameCount++;
	}
}

int DX11Renderer::StartTest()
{
	_frameCount = 0;
	_frameTimes = 0.0f;
	_testRunning = true;
	return 0;
}

float DX11Renderer::EndTest()
{
	float avgTime = _frameTimes / _frameCount;

	_frameCount = 0;
	_testRunning = false;
	return avgTime;
}

DX11Renderer::MeshHandle DX11Renderer::CreateMesh(const std::string & filename)
{
	//this should really not be in the renderer but it is in the vulkan implementation
	//This project is just for testing purposes anyway so w/e

	std::ifstream fin(filename);
	if (!fin.is_open())
	{
		throw std::exception("Failed to find mesh file");
	}

	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<XMFLOAT2> texcoords;

	std::vector<unsigned> positionIndices;
	std::vector<unsigned> normalIndices;
	std::vector<unsigned> texcoordIndices;

	std::vector<Vertex> finishedVertices;
	std::vector<unsigned> finishedIndices;

	if (filename.substr(filename.size() - 3) == "obj")
	{


		for (std::string line; std::getline(fin, line);)
		{
			std::istringstream input(line);
			std::string type;
			input >> type;

			if (type == "v")
			{
				XMFLOAT3 pos;
				input >> pos.x >> pos.y >> pos.z;
				positions.push_back(pos);
			}
			else if (type == "vt")
			{
				XMFLOAT2 tex;
				input >> tex.x >> tex.y;
				texcoords.push_back(tex);
			}
			else if (type == "vn")
			{
				XMFLOAT3 normal;
				input >> normal.x >> normal.y >> normal.z;
				normals.push_back(normal);
			}
			else if (type == "f")
			{
				int pos, tex, nor;
				char garbage;
				for (int i = 0; i < 3; ++i)
				{
					input >> pos >> garbage >> tex >> garbage >> nor;
					positionIndices.push_back(pos);
					texcoordIndices.push_back(tex);
					normalIndices.push_back(nor);

				}
			}
		}
	}
	else
	{
		throw std::exception("Unknown file format for mesh");
	}

	std::vector<XMFLOAT3> realPos;
	realPos.reserve(positionIndices.size());
	std::vector<XMFLOAT2> realTex;
	realTex.reserve(texcoordIndices.size());
	std::vector<XMFLOAT3> realNor;
	realNor.reserve(normalIndices.size());



	for (auto& pos : positionIndices)
	{
		realPos.push_back(positions[pos - 1]);
	}
	for (auto& tex : texcoordIndices)
	{
		realTex.push_back(texcoords[tex - 1]);
	}
	for (auto& nor : normalIndices)
	{
		realNor.push_back(normals[nor - 1]);
	}
	std::vector<XMFLOAT4> tan1;
	std::vector<XMFLOAT4> tan2;
	tan1.resize(realNor.size(), XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	tan2.resize(realNor.size(), XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

	std::vector<XMFLOAT4> realTan;
	realTan.resize(realNor.size(), XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
	for (unsigned i = 0; i < positionIndices.size(); i += 3)
	{
		const XMFLOAT3& v1 = realPos[i];
		const XMFLOAT3& v2 = realPos[i + 1];
		const XMFLOAT3& v3 = realPos[i + 2];

		const XMFLOAT2& u1 = realTex[i];
		const XMFLOAT2& u2 = realTex[i + 1];
		const XMFLOAT2& u3 = realTex[i + 2];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = u2.x - u1.x;
		float s2 = u3.x - u1.x;
		float t1 = u2.y - u1.y;
		float t2 = u3.y - u1.y;

		float r = 1.0f / (s1 * t2 - s2 * t1);

		XMFLOAT3 sdir = XMFLOAT3((t2*x1 - t1*x2) * r, (t2*y1 - t1*y2)*r, (t2*z1 - t1*z2)*r);
		XMFLOAT3 tdir = XMFLOAT3((s1*x2 - s2*x1)*r, (s1*y2 - s2*y1)*r, (s1*z2 - s2*z1)*r);

		tan1[i].x += sdir.x;
		tan1[i].y += sdir.y;
		tan1[i].z += sdir.z;

		tan2[i].x += tdir.x;
		tan2[i].y += tdir.y;
		tan2[i].z += tdir.z;

		tan1[i + 1].x += sdir.x;
		tan1[i + 1].y += sdir.y;
		tan1[i + 1].z += sdir.z;

		tan2[i + 1].x += tdir.x;
		tan2[i + 1].y += tdir.y;
		tan2[i + 1].z += tdir.z;

		tan1[i + 2].x += sdir.x;
		tan1[i + 2].y += sdir.y;
		tan1[i + 2].z += sdir.z;

		tan2[i + 2].x += tdir.x;
		tan2[i + 2].y += tdir.y;
		tan2[i + 2].z += tdir.z;


	}

	for (unsigned i = 0; i < realPos.size(); ++i)
	{
		XMVECTOR n = XMLoadFloat3(&realNor[i]);
		XMVECTOR t = XMLoadFloat4(&tan1[i]);

		XMVECTOR tangent = XMVector3Normalize(t - n * XMVector3Dot(n, t));

		XMStoreFloat4(&realTan[i], tangent);
		realTan[i].w = XMVectorGetX(XMVector3Dot(XMVector3Cross(n, t), XMLoadFloat4(&tan2[i]))) < 0.0f ? -1.0f : 1.0f;
	}

	finishedVertices.resize(realPos.size());

	for (unsigned i = 0; i < realPos.size(); ++i)
	{
		finishedVertices[i].pos = realPos[i];
		finishedVertices[i].nor = realNor[i];
		finishedVertices[i].tex = realTex[i];
	}

	MeshStuff ms;
	ms.vertexCount = finishedVertices.size();
	ms.indexCount = finishedVertices.size();
	ms.vertexBuffer = _CreateVertexBuffer(finishedVertices.data(), finishedVertices.size());
	ms.indexBuffer = 0;// _CreateIndexBuffer(finishedIndices.data(), finishedIndices.size());

	_meshes.push_back(ms);
	
	return _meshes.size() - 1;
}

uint32_t DX11Renderer::CreateTexture(const char * path)
{
	ID3D11ShaderResourceView* srv = nullptr;
	std::string filename(path);
	std::wstring ws(filename.begin(), filename.end());
	if (ws.substr(ws.size() - 4) == L".dds")
	{
		HRESULT hr = CreateDDSTextureFromFile(_device, ws.c_str() , nullptr, &srv);
		if (FAILED(hr))
		{
			
			throw std::exception("Texture failed");
		}
	}
	else if (ws.substr(ws.size() - 4) == L".png" || ws.substr(ws.size() -4 ) == L".jpg")
	{
		HRESULT hr = CreateWICTextureFromFile(_device, ws.c_str(), nullptr, &srv);
		if (FAILED(hr))
		{
			throw std::exception("Texture failed");
		}
	}
	else
	{
		throw std::exception("Texture failed");
	}
	_textures.push_back(srv);
	return _textures.size() - 1;
}

DX11Renderer::TranslationHandle DX11Renderer::CreateTranslation(const DirectX::XMMATRIX & translation)
{
	_translations.push_back(translation);
	return _translations.size() - 1;
}

void DX11Renderer::Submit(MeshHandle mesh, TextureHandle texture, TranslationHandle translation)
{
	_renderJobs.push_back({ mesh, translation, texture });
}

void DX11Renderer::UpdateTranslation(const DirectX::XMMATRIX & translation, TranslationHandle translationHandle)
{
	if (translationHandle >= _translations.size())
		throw std::exception("Dumb shit");
	_translations[translationHandle] = translation;
}

void DX11Renderer::SetViewMatrix(const DirectX::XMMATRIX & view)
{
	XMStoreFloat4x4(&_vp.view, view);
}

void DX11Renderer::SetProjectionMatrix(const DirectX::XMMATRIX & projection)
{
	XMStoreFloat4x4(&_vp.proj, projection);
}

float DX11Renderer::GetAspect()
{
	return _width / _height;
}

void DX11Renderer::FrustumCull(VkCommandBuffer & buffer, uint32_t start, uint32_t count) const
{
	//Dont do anything. Have to refactor code for vulkan renderer to not have these public
}

void DX11Renderer::RecordDrawCalls(VkCommandBuffer & buffer, uint32_t start, uint32_t count) const
{
	//Dont do anything. Have to refactor code for vulkan renderer to not have these public
}



void DX11Renderer::_CreateShadersAndInputLayouts()
{
	ID3DBlob* pVS;
	D3DCompileFromFile(L"vertex.hlsl", nullptr, nullptr, "main", "vs_5_0",
		NULL, NULL, &pVS, nullptr);
	_device->CreateVertexShader(pVS->GetBufferPointer(), pVS->GetBufferSize(), nullptr, &_vertexShader);

	D3D11_INPUT_ELEMENT_DESC id[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	_device->CreateInputLayout(id, ARRAYSIZE(id), pVS->GetBufferPointer(), pVS->GetBufferSize(), &_inputLayout);
	SAFE_RELEASE(pVS);
	HRESULT hr;
	

	D3DCompileFromFile(L"pixel.hlsl", nullptr, nullptr, "main", "ps_4_0",
		NULL, NULL, &pVS, nullptr);
	_device->CreatePixelShader(pVS->GetBufferPointer(), pVS->GetBufferSize(), nullptr, &_pixelShader);
	SAFE_RELEASE(pVS);

	

}

void DX11Renderer::_CreateDepthBuffer()
{
	D3D11_TEXTURE2D_DESC dsd;
	ZeroMemory(&dsd, sizeof(dsd));
	dsd.Width = _width;
	dsd.Height = _height;
	dsd.MipLevels = 1;
	dsd.ArraySize = 1;
	dsd.Format = DXGI_FORMAT_R24G8_TYPELESS;
	dsd.SampleDesc.Count = 1;
	dsd.SampleDesc.Quality = 0;
	dsd.Usage = D3D11_USAGE_DEFAULT;
	dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	dsd.CPUAccessFlags = 0;
	dsd.MiscFlags = 0;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));
	dsvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	ZeroMemory(&srvd, sizeof(srvd));
	srvd.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MostDetailedMip = 0;
	srvd.Texture2D.MipLevels = -1;


	_device->CreateTexture2D(&dsd, 0, &_dsb);
	_device->CreateDepthStencilView(_dsb, &dsvd, &_dsv);
}

void DX11Renderer::_CreateSamplerState()
{
	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MaxAnisotropy = 16;
	sd.Filter = D3D11_FILTER_ANISOTROPIC;
	sd.MinLOD = -FLT_MAX;
	sd.MaxLOD = FLT_MAX;
	sd.MipLODBias = 0.0f;
	_device->CreateSamplerState(&sd, &_samplerState);
	_deviceContext->PSSetSamplers(0, 1, &_samplerState);
}

void DX11Renderer::_CreateViewPort()
{
	D3D11_VIEWPORT vp;
	vp.Width = _width;
	vp.Height = _height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	_deviceContext->RSSetViewports(1, &vp);
}

void DX11Renderer::_CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rd;
	rd.AntialiasedLineEnable = false;
	rd.CullMode = D3D11_CULL_NONE;
	rd.FrontCounterClockwise = true;
	rd.DepthBias = false;
	rd.SlopeScaledDepthBias = 0.0f;
	rd.DepthClipEnable = true;
	rd.DepthBiasClamp = 0.0f;
	rd.FillMode = D3D11_FILL_SOLID;
	rd.MultisampleEnable = false;
	rd.ScissorEnable = false;

	HRESULT hr;
	hr = _device->CreateRasterizerState(&rd, &_rasterizerState);

	_deviceContext->RSSetState(_rasterizerState);
}



void DX11Renderer::_CreateConstantBuffers()
{
	HRESULT hr;
	D3D11_BUFFER_DESC bd;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.StructureByteStride = 0;
	bd.MiscFlags = 0;

	bd.ByteWidth = sizeof(PerFrameBuffer);
	_device->CreateBuffer(&bd, nullptr, &_constantBuffers[ConstantBuffers::CB_PER_FRAME]);

	bd.ByteWidth = sizeof(PerObjectBuffer);
	_device->CreateBuffer(&bd, nullptr, &_constantBuffers[ConstantBuffers::CB_PER_OBJECT]);

}

int DX11Renderer::_CreateVertexBuffer(Vertex * vertexData, unsigned vertexCount)
{
	D3D11_BUFFER_DESC bd;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = sizeof(Vertex) * vertexCount;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	bd.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = &vertexData[0];
	ID3D11Buffer* buffer = nullptr;
	HRESULT hr = _device->CreateBuffer(&bd, &data, &buffer);
	if (FAILED(hr))
	{
		throw std::exception("Vertex buffer failed");
	}
	_vertexBuffers.push_back(buffer);
	return _vertexBuffers.size() - 1;
}

int DX11Renderer::_CreateIndexBuffer(unsigned * indexData, unsigned indexCount)
{
	D3D11_BUFFER_DESC bd;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.ByteWidth = sizeof(unsigned) * indexCount;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	bd.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = &indexData[0];
	ID3D11Buffer* buffer = nullptr;
	HRESULT hr = _device->CreateBuffer(&bd, &data, &buffer);
	if (FAILED(hr))
	{
		throw std::exception("Index buffer failed");
	}
	_indexBuffers.push_back(buffer);

	return _indexBuffers.size() - 1;
}