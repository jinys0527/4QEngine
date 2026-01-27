#pragma once
#include "StatComponent.h"

class EnemyStatComponent : public StatComponent
{
public:
	static constexpr const char* StaticTypeName = "EnemyStatComponent";
	const char* GetTypeName() const override;

	EnemyStatComponent() = default;
	~EnemyStatComponent() override = default;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const int&   GetDefense() const						 { return m_Defense;			 }
	void	     SetDefense(const int& value)			 { m_Defense = value;			 }
			     
	const int&   GetInitiativeModifier() const			 { return m_InitiativeModifier;  }
	void	     SetInitiativeModifier(const int& value) { m_InitiativeModifier = value; }
			     
	const int&   GetAccuracyModifier() const		     { return m_AccuracyModifier;    }
	void	     SetAccuracyModifier(const int& value)   { m_AccuracyModifier = value;   }

	const int&   GetDiceRollCount() const				 { return m_DiceRollCount;		 }
	void	     SetDiceRollCount(const int& value)		 { m_DiceRollCount = value;		 }

	const int&   GetMaxDiceValue() const				 { return m_MaxDiceValue;		 }
	void	     SetMaxDiceValue(const int& value)		 { m_MaxDiceValue = value;		 }
					     
	const float& GetSightDistance() const			     { return m_SightDistance;       }
	void		 SetSightDistance(const float& value)    { m_SightDistance = value;      }
													     								 
	const float& GetSightAngle() const				     { return m_SightAngle;	         }
	void		 SetSightAngle(const float& value)       { m_SightAngle = value;	     }
													     								 
	const int&   GetDifficultyGroup() const			     { return m_DifficultyGroup;     }
	void	     SetDifficultyGroup(const int& value)    { m_DifficultyGroup = value;    }

private:
	int   m_Defense			   = 0;
	int   m_InitiativeModifier = 0;
	int   m_AccuracyModifier   = 0;
	int   m_DiceRollCount      = 0;
	int   m_MaxDiceValue       = 0;
	float m_SightDistance      = 5.0f;
	float m_SightAngle         = 90.0f;
	int   m_DifficultyGroup    = 1;
};

