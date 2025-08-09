#pragma once

#include "znmsp.h"

#include <functional>
#include <unordered_map>
#include <typeindex>

#include <Core/Logger.h>
#include "Events.h"

BEGIN_ENGINE

template<typename T>
using EventFn = std::function<void(const T&)>;

enum EventFlagBits
{
	EF_NONE = 0,
	EF_REMOVE_ON_SCENE_LOAD = 1 << 0
};

class EventBus {

	class IEventListener
	{
	public:
		EventFlagBits Flags;
		virtual ~IEventListener() = default;
	};

	template<typename T>
	class EventListener : public IEventListener
	{
	public:
		EventFn<T> FnStor;
		EventListener(EventFn<T> handler, EventFlagBits flags = EF_NONE)
		{
			this->Flags = flags;
			this->FnStor = std::move(handler);
		}
	};

public:

	static void Process();

	template<typename T>
	static void InvokeEvent(const T& event)
	{
		const type_info& typeidx = typeid(T);
		if (auto e = s_EventHandlers.find(typeidx); e != s_EventHandlers.end())
		{
			for (auto & listener : e->second)
			{
				static_cast<EventListener<T>*>(listener)->FnStor(event);
			}
		}
	}

	template<typename T>
	static void RegisterListener(EventFn<T> handler, EventFlagBits flags)
	{
		auto h = new EventListener<T>(handler, flags);
		const type_info& typeidx = typeid(T);
		s_EventHandlers[typeidx].push_back(static_cast<IEventListener*>(h));
	}

	static void RemoveSceneEventListeners();

private:

	inline static std::unordered_map<std::type_index, std::vector<IEventListener*>> s_EventHandlers;

};

END_ENGINE