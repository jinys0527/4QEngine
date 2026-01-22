#include "Component.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "DirectionalLightComponent.h"
#include "PointLightComponent.h"
#include "SpotLightComponent.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "UIComponent.h"
#include "AnimationComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMeshRenderer.h"
#include "PlayerMovementComponent.h"
#include "FSMComponent.h"
#include "CollisionFSMComponent.h"
#include "AnimFSMComponent.h"
#include "UIFSMComponent.h"
#include "SceneChangeTestComponent.h"

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
	void Link_DirectionalLightComponent();
	void Link_PointLightComponent();
	void Link_SpotLightComponent();
	void Link_UIComponent();
	void Link_AnimationComponent();
	void Link_SkeletalMeshComponent();
	void Link_SkeletalMeshRenderer();
	void Link_PlayerMovementComponent();
	void Link_FSMComponent();
	void Link_CollisionFSMComponent();
	void Link_AnimFSMComponent();
	void Link_UIFSMComponent();
	void Link_SceneChangeTestComponent();
}

void RegisterUIFSMDefinitions();
void RegisterCollisionFSMDefinitions();
void RegisterAnimFSMDefinitions();
void RegisterFSMBaseDefinitions();

//해당 함수는 client.exe에서 한번 호출로 component들의 obj 를 가져올 명분제공
void LinkEngineComponents() {

	Link_TransformComponent();
	Link_MeshRenderer();
	Link_CameraComponent();
	Link_MaterialComponent();
	Link_MeshComponent();
	Link_DirectionalLightComponent();
	Link_PointLightComponent();
	Link_SpotLightComponent();
	Link_UIComponent();
	Link_AnimationComponent();
	Link_SkeletalMeshComponent();
	Link_SkeletalMeshRenderer();
	Link_PlayerMovementComponent();
	Link_FSMComponent();
	Link_CollisionFSMComponent();
	Link_AnimFSMComponent();
	Link_UIFSMComponent();
	Link_SceneChangeTestComponent();

	RegisterFSMBaseDefinitions();
	RegisterUIFSMDefinitions();
	RegisterCollisionFSMDefinitions();
	RegisterAnimFSMDefinitions();
}