
#include <SDL.h>
#include "Scene.h"
#include <stdexcept>
#include <ConsoleThread.h>
#include <SDL_syswm.h>
#include <Parsers.h>
#include "Timer.h"
#include "InputManager.h"
#include "Renderer.h"
#include "DX11Renderer.h"
#define SCRWIDTH (800)
#define SCRHEIGHT (640)

#define IF_KEY_DOWN(keycode) if(input.IsKeyDown(keycode))

#pragma comment(lib, "DebugConsole.lib")

int main(int argc, char** argv)
{
	DebugUtils::DebugConsole::Command_Structure def =
	{
		nullptr,
		[](void* userData, int argc, char** argv) {printf("Command not found: %s\n\n", argv[0]);},
		[](void* userData, int argc, char** argv) {},
		"",
		""
	};


	DebugUtils::ConsoleThread::Init(&def);

	DebugUtils::DebugConsole::Command_Structure exitCommand =
	{
		nullptr,
		[](void* userData, int argc, char** argv) {
		SDL_Event user_event;

		user_event.type = SDL_QUIT;
		user_event.user.code = 0;
		user_event.user.data1 = NULL;
		user_event.user.data2 = NULL;
		SDL_PushEvent(&user_event);
	},
		[](void* userData, int argc, char** argv) {},
		"exit",
		"Terminates the program."
	};

	DebugUtils::ConsoleThread::AddCommand(&exitCommand);

	DebugUtils::DebugConsole::Command_Structure parseObjCmd =
	{
		nullptr,
		[](void* userData, int argc, char** argv) {
		ArfData::Data data;
		ArfData::DataPointers datap;
		int res = ParseObj(argv[1], &data, &datap);


		printf("Result: %d\n", res);
	},
		[](void* userData, int argc, char** argv) {},
		"Parse",
		"Parses an obj."
	};

	DebugUtils::ConsoleThread::AddCommand(&parseObjCmd);

	DebugUtils::ConsoleThread::ShowConsole();


	





	try
	{

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
			exit(1);
		SDL_Window* sdlWindow = nullptr;
		if ((sdlWindow = SDL_CreateWindow("Vulkan Project", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCRWIDTH, SCRHEIGHT, SDL_WINDOW_SHOWN)) == nullptr)
			exit(2);

		//Stuff we need to pass to vulkan in order to create the swapchain:
		SDL_SysWMinfo winfo;
		SDL_VERSION(&winfo.version);
		SDL_GetWindowWMInfo(sdlWindow, &winfo);
		HWND hwnd = winfo.info.win.window; //This one

		TCHAR className[256];
		GetClassName(hwnd, className, 256);
		WNDCLASS wndClass;
		GetClassInfo(GetModuleHandle(NULL), className, &wndClass);
		//Vulkan needs "wndClass.hInstance" and "hwnd"

		IRenderer* renderer = new DX11Renderer(hwnd, SCRWIDTH, SCRHEIGHT);

	

		Scene scene(renderer, SCRWIDTH, SCRHEIGHT);

		scene.Init();
		Timer timer;
		timer.Reset();
		timer.Start();
		bool quit = false;
		InputManager input;
		do
		{
			input.Update();
			timer.Tick();
			float dt = timer.DeltaTime();
			const float degreesPerSecond = 180.0f;

			if (input.IsKeyDown(SDLK_w))
				scene._camera.MoveForward(dt * 10.0f);
			if (input.IsKeyDown(SDLK_a))
				scene._camera.MoveRight(dt * -10.0f);
			if (input.IsKeyDown(SDLK_s))
				scene._camera.MoveForward(dt * -10.0f);
			if (input.IsKeyDown(SDLK_d))
				scene._camera.MoveRight(dt * 10.0f);
			if (input.IsKeyDown(SDLK_RIGHT))
				scene._camera.RotateYaw(degreesPerSecond * dt);
			if (input.IsKeyDown(SDLK_LEFT))
				scene._camera.RotateYaw(-degreesPerSecond * dt);
			if (input.IsKeyDown(SDLK_UP))
				scene._camera.RotatePitch(degreesPerSecond * dt);
			if (input.IsKeyDown(SDLK_DOWN))
				scene._camera.RotatePitch(-degreesPerSecond * dt);
			IF_KEY_DOWN(SDLK_LSHIFT)
				scene._camera.MoveUp(dt*10.0f);
			IF_KEY_DOWN(SDLK_LCTRL)
				scene._camera.MoveUp(dt * -10.0f);
			if (input.IsKeyDown(SDLK_ESCAPE))

				quit = true;
			

			scene.Frame(timer.DeltaTime());

			
		} while (!quit);

		delete renderer;
		scene.Shutdown();

		SDL_Quit();
	}
	catch (const std::runtime_error& err)
	{
		printf(err.what());
		getchar();
	}


	DebugUtils::ConsoleThread::Shutdown();
	return 0;
}