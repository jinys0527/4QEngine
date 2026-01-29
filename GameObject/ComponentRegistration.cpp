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
#include "SkinningAnimationComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMeshRenderer.h"
#include "FSMComponent.h"
#include "CollisionFSMComponent.h"
#include "AnimFSMComponent.h"
#include "UIFSMComponent.h"
#include "BoxColliderComponent.h"
#include "SceneChangeTestComponent.h"
#include "PlayerMovementComponent.h"
#include "GridSystemComponent.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "InputEventTestComponent.h"
#include "StatComponent.h"
#include "PlayerStatComponent.h"
#include "EnemyStatComponent.h"

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
	void Link_SkinningAnimationComponent();
	void Link_SkeletalMeshComponent();
	void Link_SkeletalMeshRenderer();
	void Link_FSMComponent();
	void Link_CollisionFSMComponent();
	void Link_AnimFSMComponent();
	void Link_UIFSMComponent();
	void Link_BoxColliderComponent();
	
	//User Defined
	void Link_SceneChangeTestComponent();
	void Link_PlayerMovementComponent();
	void Link_EnemyMovementComponent();
	void Link_GridSystemComponent();
	void Link_NodeComponent();
	void Link_PlayerComponent();
	void Link_EnemyComponent();
	void Link_InputEventTestComponent();
	void Link_StatComponent();
	void Link_PlayerStatComponent();
	void Link_EnemyStatComponent();
	void Link_EnemyControllerComponent();
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
	Link_SkinningAnimationComponent();
	Link_SkeletalMeshComponent();
	Link_SkeletalMeshRenderer();
	Link_FSMComponent();
	Link_CollisionFSMComponent();
	Link_AnimFSMComponent();
	Link_UIFSMComponent();
	Link_SceneChangeTestComponent();
	Link_BoxColliderComponent();


	RegisterFSMBaseDefinitions();
	RegisterUIFSMDefinitions();
	RegisterCollisionFSMDefinitions();
	RegisterAnimFSMDefinitions();


	//User Defined
	Link_SceneChangeTestComponent();
	Link_PlayerMovementComponent();
	Link_EnemyMovementComponent();
	Link_GridSystemComponent();
	Link_NodeComponent();
	Link_PlayerComponent();
	Link_EnemyComponent();
	Link_InputEventTestComponent();
	Link_StatComponent();
	Link_PlayerStatComponent();
	Link_EnemyStatComponent();
	Link_EnemyControllerComponent();
}