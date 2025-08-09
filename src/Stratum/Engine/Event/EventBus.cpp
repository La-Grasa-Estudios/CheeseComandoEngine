#include "EventBus.h"

using namespace ENGINE_NAMESPACE;

void EventBus::RemoveSceneEventListeners()
{
	for (auto& handler : s_EventHandlers)
	{
		auto& v = handler.second;

		for (uint32_t i = 0; i < v.size(); ++i)
		{
			if (v[i]->Flags & EF_REMOVE_ON_SCENE_LOAD)
			{
				auto ptr = v[i];
				if (v.size() == 1)
				{
					v.clear();
					break;
				}
				v.erase(v.begin() + i);
				i--;
				delete ptr;
			}
		}
	}
}