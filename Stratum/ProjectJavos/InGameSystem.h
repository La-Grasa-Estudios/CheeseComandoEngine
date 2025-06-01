#pragma once

#include "Common.h"
#include "CharaSprite.h"

#include <Scene/Scene.h>
#include <Sound/AudioSystem.h>

#include <atomic>

namespace Funkin
{
	class Conductor;

	class InGameSystem : public Stratum::ISceneSystem
	{
	public:

		InGameSystem(const LoadChartParams& params);
		~InGameSystem() override;

		void Init(Stratum::Scene* scene) final;
		void OnActivate(Stratum::Scene* scene) final;

		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;
		void SetPlayerCharacter(CharaSprite* chara);
		void SetOpponentCharacter(CharaSprite* chara);
		CharaSprite* GetPlayerCharacter();
		CharaSprite* GetOpponentCharacter();

		float GetLoadingProgress();
		bool IsLoadingDone();

	private:

		void UpdateStage();

		LoadChartParams mLoadParams;
		Stratum::ECS::edict_t mPlayerSprite;

		Stratum::ECS::edict_t mWhiteSprite;
		float mFadeToWhiteTime = 0.0f;
		float mFadeToWhiteBaseTime = 0.0f;
		float mFadeToWhiteIntensity = 0.0f;

		Stratum::Ref<Stratum::MP3AudioSource> instSource;
		Stratum::Ref<Stratum::MP3AudioSource> voicesSource;
		Stratum::Ref<Stratum::MP3AudioSource> missSources[3];

		glm::vec2 CameraOffsets[2];

		Stratum::Scene* mScene;
		Conductor* mConductor;
		CharaSprite* mPlayerCharacter = NULL;
		CharaSprite* mOponentCharacter = NULL;

		bool mHasSongStarted = false;

		std::atomic_uint mLoadingStage;
		std::atomic_bool mLoadingDone;
	};
}