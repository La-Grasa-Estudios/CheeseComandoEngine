#pragma once

#include <Scene/Scene.h>

#include "GameState.h"
#include "CharaSprite.h"

namespace Funkin
{
	class Conductor;

	struct PlayerComponent
	{
		Stratum::ECS::edict_t bfEntity;
		bool doBeat = false;
		float beatAcumulator;
		uint32_t frameIndex = 0;
	};

	class PlayerSystem : public Stratum::ISceneSystem
	{
	public:
		
		PlayerSystem(Conductor* conductor, GameState* gameState);
		void Init(Stratum::Scene* scene) override;
		void Update(Stratum::Scene* scene) override;
		void PostUpdate(Stratum::Scene* scene) override;
		void SetCharacter(CharaSprite* pCharaSprite);

	private:

		Stratum::Scene* mScene;
		Conductor* mConductor;
		GameState* mGameState;
		CharaSprite* mCharaSprite = NULL;

	};
}