//editor
#include "pch.h"
#include "EditorApplication.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "UIManager.h"
#include "Engine.h"
#include "AssetLoader.h"
#include "Renderer.h"
#include "Importer.h"


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
	Engine engine;
	SoundManager soundManager;
	UIManager uiManager(engine.GetEventDispatcher());
	SceneManager sceneManager(engine.GetEventDispatcher(), /*engine.GetAssetManager(), engine.GetSoundAssetManager(), */soundManager, uiManager);
	AssetLoader assetLoader;
	Renderer renderer(assetLoader);
	 //<<-- FrameData 강제 필요 but imgui 는 필요 없음
	//Editor는 시작시 사용하는 모든 fbx load
	//ImportFBX("../Dying.fbx", "../test");
	//ImportFBX("../Unarmed Walk Forward.fbx", "../test");

	EditorApplication app(engine,renderer, sceneManager, soundManager);
	if (!app.Initialize())
	{
		CoUninitialize();
		return -1;
	}


	// 구동
	app.Run();
	app.Finalize();
	//


	//uiManager.Reset();
	sceneManager.Reset();
	engine.Reset();

	CoUninitialize();

	return 0;
}