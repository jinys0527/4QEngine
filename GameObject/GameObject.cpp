#include "Component.h"
#include "GameObject.h"


GameObject::GameObject(EventDispatcher& eventDispatcher) : Object(eventDispatcher)
{
	//m_Transform = AddComponent<TransformComponent>();
}

//bool GameObject::IsInView(CameraObject* camera) const
//{
//	// 카메라 뷰 영역 크기, 위치 가져오기
//	// 카메라 위치 (World)
//	auto cameraComp = camera->GetComponent<CameraComponent>();
//	auto cameraPos = camera->GetComponent<TransformComponent>()->GetPosition();
//
//	float cameraWidth = cameraComp->GetWidth() / cameraComp->GetZoom();
//	float cameraHeight = cameraComp->GetHeight() / cameraComp->GetZoom();
//
//	// 카메라 뷰의 Rect
//	float cameraLeft = cameraPos.x - cameraWidth * 0.5f;
//	float cameraTop = cameraPos.y - cameraHeight * 0.5f;
//	float cameraRight = cameraLeft + cameraWidth;
//	float cameraBottom = cameraTop + cameraHeight;
//
//	// 오브젝트 위치 및 크기 가져오기
//	auto trans = GetComponent<TransformComponent>();
//
//	if (!trans)
//		return true;
//
//	auto spriteRenderer = GetComponent<SpriteRenderer>();
//
//	if (!spriteRenderer)
//		return false;
//
//	auto objPos = trans->GetPosition();
//	if (trans->GetParent())
//	{
//		objPos += trans->GetParent()->GetPosition();
//	}
//	if (!spriteRenderer->GetTexture())
//		return false;
//	auto objSize = spriteRenderer->GetTexture()->GetSize();
//	/*auto objPivot = spriteRenderer->GetPivot();*/
//
//	// 오브젝트 좌상단 좌표 계산 (pivot 적용)
//	float objLeft = objPos.x - objSize.width * 0.5f;
//	float objTop = objPos.y;
//	float objRight = objPos.x + objSize.width * 0.5f;
//	float objBottom = objPos.y + objSize.height;
//
//	// AABB 충돌 검사
//	bool isVisible = !(objRight < cameraLeft || objLeft > cameraRight ||
//		objBottom < cameraTop || objTop > cameraBottom);
//
//	return isVisible;
//}

//void GameObject::Render(std::vector<RenderInfo>& renderInfo)
//{
//	for(auto& spriteRenderer : GetComponents<SpriteRenderer>())
//	{
//		RenderInfo info;
//		auto box = GetComponent<BoxColliderComponent>();
//		if (box)
//		{
//			info.center = box->GetCenter();
//			info.size = box->GetSize();
//		}
//		info.bitmap = spriteRenderer->GetTexture();
//		info.worldMatrix = m_Transform->GetWorldMatrix();
//		info.pivot = spriteRenderer->GetPivot();
//		// Opacity 적용
//		info.opacity = spriteRenderer->GetOpacity();
//		info.layer = m_Transform->GetZOrder();
//		// UI가 아닌 일반 오브젝트 위치로 설정
//		info.useSrcRect = spriteRenderer->GetUseSrcRect();
//		info.srcRect = spriteRenderer->GetSrcRect();
//		renderInfo.emplace_back(info);
//	}
//}

void GameObject::Serialize(nlohmann::json& j) const
{
	j["name"] = m_Name;
	j["components"] = nlohmann::json::array();
	for (const auto& component : m_Components)
	{
		for (const auto& comp : component.second)
		{
			const char* typeName = comp->GetTypeName();
			if (strcmp(typeName, "TransformComponent") == 0)
			{
				nlohmann::json compJson;
				compJson["type"] = typeName;
				comp->Serialize(compJson["data"]);
				j["components"].push_back(compJson);
			}
			else if(strcmp(typeName, "SpriteRenderer") == 0)
			{
				nlohmann::json compJson;
				compJson["type"] = typeName;
				comp->Serialize(compJson["data"]);
				j["components"].push_back(compJson);
			}
		}
	}
}

void GameObject::Deserialize(const nlohmann::json& j)
{
	m_Name = j.at("name");

	for (const auto& compJson : j.at("components"))
	{
		std::string typeName = compJson.at("type");

		// 기존 컴포넌트가 있으면 찾아서 갱신
		auto it = m_Components.find(typeName);
		if (it != m_Components.end() && !it->second.empty())
		{
			// 해당 타입의 모든 컴포넌트에 대해 Deserialize 시도
			for (auto& comp : it->second)
			{
				comp->Deserialize(compJson.at("data"));
			}
		}
		else
		{
			// 없으면 새로 생성 후 추가
			auto comp = ComponentFactory::Instance().Create(typeName);
			if (comp)
			{
				comp->Deserialize(compJson.at("data"));
				AddComponent(std::move(comp));
			}
		}
	}
}