#include "Player.h"

#include "Conductor.h"
#include "SparrowReader.h"

#include <Util/Globals.h>
#include <Event/EventHandler.h>

Funkin::PlayerSystem::PlayerSystem(Conductor* conductor, GameState* gameState)
{
	mConductor = conductor;
	mGameState = gameState;
	mScene = nullptr;
}

void Funkin::PlayerSystem::Init(Stratum::Scene* scene)
{
	mScene = scene;
	scene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<PlayerComponent>(), "player_component");
}

void Funkin::PlayerSystem::OnActivate(Stratum::Scene* scene)
{
	auto animationListener = [this](void* sender, void** args, uint32_t argc)
		{
			uint32_t noteType = (uint32_t)args[0];

			const char* animations[4] =
			{
				"left",
				"down",
				"up",
				"right"
			};

			mCharaSprite->PlayAnimation(animations[noteType]);

		};

	Stratum::EventHandler::RegisterListener(animationListener, Stratum::EventHandler::GetEventID("hit_note"), true, true);
	Stratum::EventHandler::RegisterListener(animationListener, Stratum::EventHandler::GetEventID("sustain_note"), true, true);
}

void Funkin::PlayerSystem::Update(Stratum::Scene* scene)
{

}

void Funkin::PlayerSystem::PostUpdate(Stratum::Scene* scene)
{
}

void Funkin::PlayerSystem::SetCharacter(CharaSprite* pCharaSprite)
{
	if (mCharaSprite)
	{
		mCharaSprite->SetEnabled(false);
	}
	mCharaSprite = pCharaSprite;
	pCharaSprite->SetEnabled(true);
}