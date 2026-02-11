#pragma once
#include "StatComponent.h"

class PlayerStatComponent : public StatComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerStatComponent";
	const char* GetTypeName() const override;

	PlayerStatComponent() = default;
	~PlayerStatComponent() override = default;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const int&   GetHealth() const						 { return m_Health;			   }
	void	     SetHealth(const int& value);
			     
	const int&   GetStrength() const					 { return m_Strength;		   }
	void		 SetStrength(const int& value);
			     
	const int&   GetAgility() const				         { return m_Agility;           }
	void	     SetAgility(const int& value);
			     
	const int&   GetSense() const						 { return m_Sense;			   }
	void	     SetSense(const int& value);
			     
	const int&   GetSkill() const						 { return m_Skill;		       }
	void	     SetSkill(const int& value);

	const int    CalculateStatModifier(int statValue) const;

	const int    GetCalculatedHealthModifier  () const { return CalculateStatModifier(m_Health);   }
	const int    GetCalculatedStrengthModifier() const { return CalculateStatModifier(m_Strength); }
	const int    GetCalculatedAgilityModifier () const { return CalculateStatModifier(m_Agility);  }
	const int    GetCalculatedSenseModifier   () const { return CalculateStatModifier(m_Sense);    }
	const int    GetCalculatedSkillModifier   () const { return CalculateStatModifier(m_Skill);    }

	float        GetShopDiscountRate          () const;
	void         SetEquipmentDefenseBonus	  (const int& value);
	const int&   GetEquipmentDefenseBonus     () const { return m_EquipmentDefenseBonus;			  }
	void         SetEquipmentDefenseBonus	  (const int& value) { m_EquipmentDefenseBonus = value; }
	const int    GetMaxHealthForFloor	 	  (int currentFloor) const;
	const int    GetDefense				      () const { return GetCalculatedSenseModifier() + m_EquipmentDefenseBonus; }

private:
	void DispatchStatChangedEvent();

	int   m_Health				  = 12;
	int   m_Strength			  = 12;
	int   m_Agility				  = 12;
	int   m_Sense				  = 12;
	int   m_Skill				  = 12;
	int   m_EquipmentDefenseBonus = 0;
};

