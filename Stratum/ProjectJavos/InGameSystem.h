#pragma once

#include "Common.h"

#include <Scene/Scene.h>
#include <Sound/AudioSystem.h>

namespace Javos
{
	class Conductor;

	class InGameSystem : public Stratum::ISceneSystem
	{
	public:

		InGameSystem(const LoadChartParams& params);
		~InGameSystem();

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;

	private:

		LoadChartParams mLoadParams;
		Stratum::ECS::edict_t mWhiteSprite;
		float mFadeToWhiteTime = 0.0f;
		float mFadeToWhiteBaseTime = 0.0f;
		float mFadeToWhiteIntensity = 0.0f;

		Stratum::Ref<Stratum::MP3AudioSource> instSource;
		Stratum::Ref<Stratum::MP3AudioSource> voicesSource;
		Stratum::Ref<Stratum::MP3AudioSource> missSources[3];

		Stratum::Scene* mScene;
		Conductor* mConductor;
	};
}