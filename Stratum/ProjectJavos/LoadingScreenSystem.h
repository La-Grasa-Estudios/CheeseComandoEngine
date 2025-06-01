#pragma once

#include "Common.h"

#include <Scene/Scene.h>
#include <Sound/AudioSystem.h>

namespace Funkin
{
	class Conductor;

	class LoadingScreenSystem : public Stratum::ISceneSystem
	{
	public:

		LoadingScreenSystem(const LoadChartParams& params);
		~LoadingScreenSystem() override;

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;

	private:

		LoadChartParams mLoadParams;

		Stratum::Ref<Stratum::MP3AudioSource> mMusicSource;

		Stratum::Scene* mScene;
		Stratum::Scene* mLoadingScene;
	};
}