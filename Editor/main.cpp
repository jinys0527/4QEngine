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
	AssetLoader assetLoader;
	Renderer renderer(assetLoader);
	SceneManager sceneManager(/*renderer,*/ engine.GetEventDispatcher(), /*engine.GetAssetManager(), engine.GetSoundAssetManager(), */soundManager, uiManager);
	 //<<-- FrameData 강제 필요 but imgui 는 필요 없음
	//Editor는 시작시 사용하는 모든 fbx load
	//ImportAll();
	//assetLoader.LoadAll();
	ImportFBX("../Resources/FBX/Dying.fbx", "../ResourceOutput");
	assetLoader.LoadAsset("../ResourceOutput/Dying/Meta/Dying.asset.json");

	EditorApplication app(engine, renderer, sceneManager, soundManager, assetLoader);
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