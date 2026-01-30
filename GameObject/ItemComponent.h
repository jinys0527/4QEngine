#pragma once
#include "Component.h"
#include <string>

class ItemComponent : public Component
{
public:
	static constexpr const char* StaticTypeName = "ItemComponent";
	const char* GetTypeName() const override;

	ItemComponent();
	virtual ~ItemComponent();

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const int& GetItemIndex() const { return m_ItemIndex; }
	void SetItemIndex(const int& value) { m_ItemIndex = value; }

	const bool& GetIsEquiped() const { return m_IsEquiped; }
	void SetIsEquiped(const bool& value) { m_IsEquiped = value; }

	const int& GetType() const { return m_Type; }
	void SetType(const int& value) { m_Type = value; }

	const int& GetName() const { return m_Name; }
	void SetName(const int& value) { m_Name = value; }

	const std::string& GetIconPath() const { return m_IconPath; }
	void SetIconPath(const std::string& value) { m_IconPath = value; }

	const std::string& GetMeshPath() const { return m_MeshPath; }
	void SetMeshPath(const std::string& value) { m_MeshPath = value; }

	const int& GetDescriptionIndex() const { return m_DescriptionIndex; }
	void SetDescriptionIndex(const int& value) { m_DescriptionIndex = value; }

	const int& GetSellPrice() const { return m_SellPrice; }
	void SetSellPrice(const int& value) { m_SellPrice = value; }

	const int& GetMeleeAttackRange() const { return m_MeleeAttackRange; }
	void SetMeleeAttackRange(const int& value) { m_MeleeAttackRange = value; }

	const int& GetMaxDiceRoll() const { return m_MaxDiceRoll; }
	void SetMaxDiceRoll(const int& value) { m_MaxDiceRoll = value; }

	const int& GetMaxDiceValue() const { return m_MaxDiceValue; }
	void SetMaxDiceValue(const int& value) { m_MaxDiceValue = value; }

	const int& GetBonusValue() const { return m_BonusValue; }
	void SetBonusValue(const int& value) { m_BonusValue = value; }

	const int& GetCON() const { return m_CON; }
	void SetCON(const int& value) { m_CON = value; }

	const int& GetSTR() const { return m_STR; }
	void SetSTR(const int& value) { m_STR = value; }

	const int& GetDEX() const { return m_DEX; }
	void SetDEX(const int& value) { m_DEX = value; }

	const int& GetSENSE() const { return m_SENSE; }
	void SetSENSE(const int& value) { m_SENSE = value; }

	const int& GetTEC() const { return m_TEC; }
	void SetTEC(const int& value) { m_TEC = value; }

	const int& GetDEF() const { return m_DEF; }
	void SetDEF(const int& value) { m_DEF = value; }

	const int& GetThrowRange() const { return m_ThrowRange; }
	void SetThrowRange(const int& value) { m_ThrowRange = value; }

	const int& GetDifficultyGroup() const { return m_DifficultyGroup; }
	void SetDifficultyGroup(const int& value) { m_DifficultyGroup = value; }

private:
	int m_ItemIndex				= -1;

	bool m_IsEquiped			= false;
	int m_Type					= -1;
	int m_Name					= -1;
	std::string m_IconPath		= "";
	std::string m_MeshPath		= "";
	int m_DescriptionIndex		= -1;
	int m_SellPrice				= -1;
	int m_MeleeAttackRange		= -1;
	int m_MaxDiceRoll			= -1;     // 주사위 굴림횟수 (DnD룰에서 1d4의 d)
	int m_MaxDiceValue			= -1;    // 주사위 면체 수 (DnD룰에서 1d4의 4)
	int m_BonusValue			= -1;      // 데미지 보정치 (고정 추가 데미지)
	int m_CON					= 0;
	int m_STR					= 0;
	int m_DEX					= 0;
	int m_SENSE					= 0;
	int m_TEC					= 0;
	int m_DEF					= 0;
	int m_ThrowRange			= -1;
	int m_DifficultyGroup		= -1;
};