
//editor
#include "pch.h"
#include "Engine.h"
#include "EditorApplication.h"
#include "SceneManager.h"
#include "SoundManager.h"
//#include "UIManager.h"
//#include "GameManager.h"
#include "Importer.h"
#include "AssetLoader.h"

namespace
{
	EditorApplication* g_pMainApp = nullptr;
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
		return -1;

	Engine engine;
	SoundManager soundManager/*engine.GetSoundAssetManager()*/;
	//GameManager gameManager(engine.GetEventDispatcher());
	//UIManager uiManager(engine.GetEventDispatcher());
	SceneManager sceneManager(/*engine.GetRenderer(),*/ engine.GetEventDispatcher(), /*engine.GetAssetManager(), engine.GetSoundAssetManager(), */soundManager, gameManager, uiManager);

	//Editor는 시작시 사용하는 모든 fbx load
	//ImportFBX("../Dying.fbx", "../test");
	//ImportFBX("../Unarmed Walk Forward.fbx", "../test");

	// 구동
	g_pMainApp->Run();

	g_pMainApp->Finalize();

	delete g_pMainApp;

	gameManager.Reset();
	//uiManager.Reset();
	sceneManager.Reset();
	engine.Reset();

	CoUninitialize();

	return 0;
}