#include "Scene.h"

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <SDL.h>
#include <ConsoleThread.h>
#include <iostream>
using namespace DirectX;

Scene::Scene(IRenderer* renderer, float width, float height) : _renderer(renderer), _camera(width, height), _testRunning(false)
{

}


Scene::~Scene()
{
}

void Scene::Init()
{
	srand(1337);
	_renderer->SetProjectionMatrix(_camera.GetProj());
	//_camera.TranslateActiveCamera(100.0f, 0.0f, -80.0f);
	_camera.SetCameraPosition(0, 0, -10);
	

	XMMATRIX t = XMMatrixTranslation(0, 0, 0);

	_CreateObject("../Assets/Meshes/deer-obj.obj", "../Assets/Textures/deer texture.png", t);
	
	auto deer = _objects[0];
	XMStoreFloat4x4(&_objects[0].translation, t);
	
	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 100; j++)
		{
			t = XMMatrixTranslation(20.0f * i, 0, 16.0f * j);
			XMStoreFloat4x4(&deer.translation, t);
			deer.translationHandle = _renderer->CreateTranslation(t);
			_objects.push_back(deer);
		}
	}

	//_CreateObject("../Assets/Meshes/bear-obj.obj", "../Assets/Textures/bear.tga");
	//_CreateObject("../Assets/Meshes/boar-obj.obj", "../Assets/Textures/boar.tga");
//	_CreateObject("../Assets/Meshes/cube2.obj", "../Assets/Textures/cube2.png", XMMatrixIdentity());


	for (auto& o : _objects)
		_renderer->Submit(o.mesh, o.texture, o.translationHandle);



	DebugUtils::DebugConsole::Command_Structure testStart =
	{
		this,
		[](void* userData, int argc, char** argv) {

		auto& scene = *(Scene*)userData;
		int res;
		char* filename = "log.log";
		DebugUtils::GetArg("-o", &filename, argc, argv);
		res = scene._StartTest(filename);

		if (res == -1)
			printf("An error occured!\n");
		else
			printf("Test Started... Output will be saved to: %s\n", filename);





	},
		[](void* userData, int argc, char** argv) {
		printf("Usage: Measure the current rendering technique.\n");
		printf("\t-o, specify the output file.\n");

	},
		"test",
		"Starts the test sequence."
	};


	DebugUtils::ConsoleThread::AddCommand(&testStart);
}

void Scene::Frame(float dt)
{
	_timer.TimeStart("Frame");

	for (auto& o : _objects)
	{
		XMMATRIX t = XMLoadFloat4x4(&o.translation);
		t *= XMMatrixTranslation((rand() % 200 - 100)/100.0f*dt, (rand() % 200 - 100) / 100.0f*dt, (rand() % 200 - 100) / 100.0f*dt);
		XMStoreFloat4x4(&o.translation, t);
		_renderer->UpdateTranslation(t, o.translationHandle);
	}


	_renderer->SetViewMatrix(_camera.GetView());

	_renderer->Render();
	_timer.TimeEnd("Frame");
	if (_testRunning)
	{
		_frameTimes += _timer.GetTime("Frame");
		_frameCount++;

		const uint32_t targetFrameCount = 100;

		if (_frameCount > targetFrameCount)
		{
			_EndTest();
		}
		else
		{
			printf("\rMeasuring: %.0f%%", 100.0f * _frameCount / targetFrameCount);
			fflush(stdout);
		}
	}
	//printf("MS: %f\n", _timer.GetTime("Frame"));
}

void Scene::Shutdown()
{
	return void();
}


void Scene::_CreateObject(const char * mesh, const char * texture, const XMMATRIX& translation)
{
	_objects.push_back({
		_renderer->CreateMesh(mesh),
		_renderer->CreateTexture(texture),
		_renderer->CreateTranslation(translation)
	});

}

int Scene::_StartTest(const char * outfile)
{
	out.open(outfile, std::ios::ate);
	if (!out.is_open())
		return -1;
	_renderer->StartTest();
	_frameCount = 0;
	_frameTimes = 0.0f;
	_testRunning = true;
	return 0;
}

void Scene::_EndTest()
{
	float avgTime = _frameTimes / _frameCount;

	_frameCount = 0;
	_testRunning = false;
	auto rendererAvg = _renderer->EndTest();
	out << "Total frametime: " << avgTime << ", Renderer: " << rendererAvg << std::endl;
	out.close();
	printf("\n Test complete, Total frametime: %f, Average frametime in renderer was: %f\n", avgTime, rendererAvg);

}
