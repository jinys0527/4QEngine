#include "Component.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "LightComponent.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "UIComponent.h"

// 중앙 등록 .cpp
// exe에서 .lib의 obj를 가져오기 위해 심볼을 연결하기 위한 것
// 비워도 됨 

// 이름 Mangling 제거 "C"
extern "C" {
	void Link_TransformComponent();
	void Link_MeshRenderer();
	void Link_CameraComponent();
	void Link_MaterialComponent();
	void Link_MeshComponent();
	void Link_LightComponent();
	void Link_UIComponent();
}
//해당 함수는 client.exe에서 한번 호출로 component들의 obj 를 가져올 명분제공
void LinkEngineComponents() {

	Link_TransformComponent();
	Link_MeshRenderer();
	Link_CameraComponent();
	Link_MaterialComponent();
	Link_MeshComponent();
	Link_LightComponent();
	Link_UIComponent();

}