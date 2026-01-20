//editor
#include "pch.h"
#include "EditorApplication.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "Engine.h"
#include "EventDispatcher.h"
#include "InputManager.h"
#include "AssetLoader.h"
#include "SoundManager.h"
#include "UIManager.h"
#include "ServiceRegistry.h"


//namespace
//{
//	EditorApplication* g_pMainApp = nullptr;
//}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		return -1;
	}

	ServiceRegistry services;

	auto& inputManager = services.Register<InputManager>();
	auto& assetLoader = services.Register<AssetLoader>();
	auto& soundManager = services.Register<SoundManager>();
	auto& uiManager = services.Register<UIManager>();

	Renderer renderer(assetLoader);
	Engine engine(services, renderer);
	SceneManager sceneManager(services);

	 //<<-- FrameData 강제 필요 but imgui 는 필요 없음
	//Editor는 시작시 사용하는 모든 fbx load
	

	EditorApplication app(services, engine, renderer, sceneManager);
	if (!app.Initialize())
	{
		CoUninitialize();
		return -1;
	}


	// 구동
	app.Run();
	app.Finalize();
	//

	sceneManager.Reset();
	engine.Reset();

	CoUninitialize();

	return 0;
}