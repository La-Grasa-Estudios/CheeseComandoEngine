#include "Player.h"

#include "Conductor.h"
#include "SparrowReader.h"
#include "Events.h"

#include <Util/Globals.h>
#include <Event/EventBus.h>

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
	auto animationListener = [this](const NoteEvent& e)
		{
			if (e.IsMiss || e.IsOponent)
				return;

			const char* animations[4] =
			{
				"left",
				"down",
				"up",
				"right"
			};

			mCharaSprite->PlayAnimation(animations[e.NoteType]);

		};

	Stratum::EventBus::RegisterListener<NoteEvent>(animationListener, Stratum::EF_REMOVE_ON_SCENE_LOAD);
	Stratum::EventBus::RegisterListener<NoteEvent>(animationListener, Stratum::EF_REMOVE_ON_SCENE_LOAD);
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