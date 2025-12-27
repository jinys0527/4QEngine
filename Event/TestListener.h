#pragma once
#include "IEventListener.h"

class TestListener : public IEventListener
{
public:
	TestListener() = default;
	~TestListener() = default;
	void OnEvent(EventType type, const void* data) override;
};

