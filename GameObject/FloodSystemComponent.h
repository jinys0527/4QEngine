#pragma once
#include "Component.h"

class FloodSystemComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "FloodSystemComponent";
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const float& GetTurnTimeLimitSeconds() const			   { return m_TurnTimeLimitSeconds;	   }
	void		 SetTurnTimeLimitSeconds(const float& seconds) { m_TurnTimeLimitSeconds = seconds; }

	const float& GetWaterLevel() const						   { return m_WaterLevel;			   }
	const float& GetWaterRisePerSecond() const				   { return m_WaterRisePerSecond;	   }
	void		 SetWaterRisePerSecond(const float& value)	   { m_WaterRisePerSecond = value;	   }

	const float& GetGameOverLevel() const					   { return m_GameOverLevel;		   }
	void		 SetGameOverLevel(const float& value)		   { m_GameOverLevel = value;		   }

	const float  GetTurnElapsed  () const					   { return m_TurnElapsed;			   }
	const float  GetTurnRemaining() const;

	const bool&  GetGameOver() const						   { return m_IsGameOver;			   }

private:
	float m_TurnElapsed			 = 0.0f;
	float m_TurnTimeLimitSeconds = 30.0f;
	float m_WaterLevel			 = 0.0f;
	float m_WaterRisePerSecond	 = 0.1f;
	float m_GameOverLevel	     = 10.0f;
	bool  m_IsGameOver			 = false;
};

