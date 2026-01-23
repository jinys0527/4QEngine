#include "UIFSMComponent.h"
#include "FSMActionRegistry.h"
#include "FSMEventRegistry.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "UIComponent.h"
#include "UIButtonComponent.h"
#include "UIProgressBarComponent.h"
#include "UISliderComponent.h"

void RegisterUIFSMDefinitions()
{
	auto& actionRegistry = FSMActionRegistry::Instance();
	actionRegistry.RegisterAction({
		"UI_SetVisible",
		"UI",
		{
			{ "value", "bool", true, false }
		}
		});
	actionRegistry.RegisterAction({
		"UI_SetOpacity",
		"UI",
		{
			{ "value", "float", 1.0f, false }
		}
		});

	actionRegistry.RegisterAction({
		"UI_SetButtonEnabled",
		"UI",
		{
			{ "value", "bool", true, false }
		}
		});
	actionRegistry.RegisterAction({
		"UI_SetSliderValue",
		"UI",
		{
			{ "value", "float", 0.0f, false }
		}
		});
	actionRegistry.RegisterAction({
		"UI_SetSliderNormalized",
		"UI",
		{
			{ "value", "float", 0.0f, false }
		}
		});
	actionRegistry.RegisterAction({
		"UI_SetProgressPercent",
		"UI",
		{
			{ "value", "float", 0.0f, false }
		}
		});

	auto& eventRegistry = FSMEventRegistry::Instance();
	eventRegistry.RegisterEvent({ "UI_Pressed", "UI" });
	eventRegistry.RegisterEvent({ "UI_Hovered", "UI" });
	eventRegistry.RegisterEvent({ "UI_Released", "UI" });
	eventRegistry.RegisterEvent({ "UI_Dragged", "UI" });
	eventRegistry.RegisterEvent({ "UI_Clicked", "UI" });
	eventRegistry.RegisterEvent({ "UI_SliderValueChanged", "UI" });
	eventRegistry.RegisterEvent({ "UI_ProgressChanged", "UI" });
}


REGISTER_COMPONENT_DERIVED(UIFSMComponent, FSMComponent)
REGISTER_PROPERTY(UIFSMComponent, EventCallbacks)
REGISTER_PROPERTY(UIFSMComponent, CallbackActions)

UIFSMComponent::UIFSMComponent()
{
	BindActionHandler("UI_SetVisible", [this](const FSMAction& action)
		{
			auto* ui = GetOwner() ? GetOwner()->GetComponent<UIComponent>() : nullptr;
			if (!ui)
				return;

			const bool visible = action.params.value("value", true);
			ui->SetVisible(visible);
		});

	BindActionHandler("UI_SetOpacity", [this](const FSMAction& action)
		{
			auto* ui = GetOwner() ? GetOwner()->GetComponent<UIComponent>() : nullptr;
			if (!ui)
				return;

			const float opacity = action.params.value("value", 1.0f);
			ui->SetOpacity(opacity);
		});

	BindActionHandler("UI_SetButtonEnabled", [this](const FSMAction& action)
		{
			auto* button = GetOwner() ? GetOwner()->GetComponent<UIButtonComponent>() : nullptr;
			if (!button)
				return;

			const bool enabled = action.params.value("value", true);
			button->SetIsEnabled(enabled);
		});

	BindActionHandler("UI_SetSliderValue", [this](const FSMAction& action)
		{
			auto* slider = GetOwner() ? GetOwner()->GetComponent<UISliderComponent>() : nullptr;
			if (!slider)
				return;

			const float value = action.params.value("value", slider->GetValue());
			slider->SetValue(value);
		});

	BindActionHandler("UI_SetSliderNormalized", [this](const FSMAction& action)
		{
			auto* slider = GetOwner() ? GetOwner()->GetComponent<UISliderComponent>() : nullptr;
			if (!slider)
				return;

			const float value = action.params.value("value", slider->GetNormalizedValue());
			slider->SetNormalizedValue(value);
		});

	BindActionHandler("UI_SetProgressPercent", [this](const FSMAction& action)
		{
			auto* progress = GetOwner() ? GetOwner()->GetComponent<UIProgressBarComponent>() : nullptr;
			if (!progress)
				return;

			const float value = action.params.value("value", progress->GetPercent());
			progress->SetPercent(value);
		});
}

UIFSMComponent::~UIFSMComponent()
{
	GetEventDispatcher().RemoveListener(EventType::Pressed, this);
	GetEventDispatcher().RemoveListener(EventType::Hovered, this);
	GetEventDispatcher().RemoveListener(EventType::Released, this);
	GetEventDispatcher().RemoveListener(EventType::Dragged, this);
}

void UIFSMComponent::Start()
{
	FSMComponent::Start();

	GetEventDispatcher().AddListener(EventType::Pressed, this);
	GetEventDispatcher().AddListener(EventType::Hovered, this);
	GetEventDispatcher().AddListener(EventType::Released, this);
	GetEventDispatcher().AddListener(EventType::Dragged, this);
}

void UIFSMComponent::OnEvent(EventType type, const void* data)
{
	const auto eventName = TranslateEvent(type, data);
	if (!eventName)
	{
		return;
	}

	HandleEventByName(*eventName, data);
}

void UIFSMComponent::RegisterCallback(const std::string& id, Callback callback)
{
	if(id.empty())
	{
		return;
	}
	m_Callbacks[id] = std::move(callback);
}

void UIFSMComponent::RegisterCallback(const std::string& id, LegacyCallback callback)
{
	if (id.empty())
	{
		return;
	}
	m_LegacyCallbacks[id] = std::move(callback);
}

void UIFSMComponent::ClearCallback(const std::string& id)
{
	m_Callbacks.erase(id);
}

void UIFSMComponent::TriggerEventByName(const std::string& eventName, const void* data)
{
	if (eventName.empty())
	{
		return;
	}
	HandleEventByName(eventName, data);
}

std::optional<std::string> UIFSMComponent::TranslateEvent(EventType type, const void* data)
{
	switch (type)
	{
	case EventType::Pressed:
		return std::string("UI_Pressed");
	case EventType::Hovered:
		return std::string("UI_Hovered");
	case EventType::Released:
		return std::string("UI_Released");
	case EventType::Dragged:
		return std::string("UI_Dragged");
	default:
		return std::nullopt;
	}
}

std::optional<EventType> UIFSMComponent::EventTypeFromName(const std::string& eventName) const
{
	if (eventName == "UI_Pressed")
	{
		return EventType::Pressed;
	}
	if (eventName == "UI_Hovered")
	{
		return EventType::Hovered;
	}
	if (eventName == "UI_Released")
	{
		return EventType::Released;
	}
	if (eventName == "UI_Dragged")
	{
		return EventType::Dragged;
	}
	return std::nullopt;
}

void UIFSMComponent::HandleEventByName(const std::string& eventName, const void* data)
{
	DispatchEvent(eventName);

	for (const auto& entry : m_EventCallbacks)
	{
		if (entry.eventName != eventName)
		{
			continue;
		}

		auto it = m_Callbacks.find(entry.callbackId);
		if (it != m_Callbacks.end())
		{
			it->second(eventName, data);
			continue;
		}

		for (const auto& actionEntry : m_CallbackActions)
		{
			if (actionEntry.callbackId != entry.callbackId)
			{
				continue;
			}
			for (const auto& action : actionEntry.actions)
			{
				HandleAction(action);
			}
			break;
		}
	}
}
