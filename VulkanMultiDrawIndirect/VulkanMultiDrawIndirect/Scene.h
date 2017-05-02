#pragma once

#include "IRenderer.h"

#include "CameraManager.h"

class Scene
{
	struct Object
	{
		IRenderer::MeshHandle mesh;
		IRenderer::TextureHandle texture;
		IRenderer::TranslationHandle translationHandle;
		IRenderer::BoundingHandle boundingHandle;
		DirectX::XMFLOAT4X4 translation;
	};
public:
	Scene(IRenderer* renderer, float width, float height);
	~Scene();


	void Init();
	void Frame(float dt);
	void Shutdown();

	CameraManager _camera;
private:
	void _CreateObject(const char* mesh, const char* texture, const DirectX::XMMATRIX& translation);

	int _StartTest(const char* outfile);
	void _EndTest();
private:
	IRenderer* _renderer;
	CPUTimer _timer;

	uint32_t _frameCount;
	float _frameTimes;
	bool _testRunning;
	std::vector<Object> _objects;

	std::ofstream out;
};

