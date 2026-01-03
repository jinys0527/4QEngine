#include "pch.h"
#include "Engine.h"
#include "GameApplication.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "UIManager.h"
#include "GameManager.h"
#include "Importer.h"

namespace
{
	GameApplication* g_pMainApp = nullptr;
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	//_CrtSetBreakAlloc(597);

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
		return -1;

    Engine engine;
    SoundManager soundManager/*engine.GetSoundAssetManager()*/;
    GameManager gameManager(engine.GetEventDispatcher());
    UIManager uiManager(engine.GetEventDispatcher());
	SceneManager sceneManager(/*engine.GetRenderer(),*/ engine.GetEventDispatcher(), /*engine.GetAssetManager(), engine.GetSoundAssetManager(), */soundManager, gameManager, uiManager);
   
    ImportFBX("../Dying.fbx", "../test");
#ifdef _EDITOR
    Editor editor(sceneManager);

    g_pMainApp = new GameApplication(engine, sceneManager, soundManager, editor);
#else
    g_pMainApp = new GameApplication(engine, sceneManager, soundManager);
#endif
 	
 
 	if (!g_pMainApp->Initialize())
 	{
 		std::cerr << "Failed to initialize sample code." << std::endl;
 		return -1;
 	}

 	g_pMainApp->Run();
 
 	g_pMainApp->Finalize();
 
 	delete g_pMainApp;
    
    gameManager.Reset();
    uiManager.Reset();
    sceneManager.Reset();
    engine.Reset();

	CoUninitialize();

	return 0;
}