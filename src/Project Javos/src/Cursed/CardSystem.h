#pragma once

#include <Scene/Scene.h>

namespace Funkin
{
	class CardSystem : public Stratum::ISceneSystem
	{
	public:

		CardSystem();
		~CardSystem();

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;

	private:

		Stratum::Scene* mScene;

	};
}