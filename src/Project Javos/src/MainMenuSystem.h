#pragma once

#include "Common.h"

#include <Input/Input.h>
#include <Scene/Scene.h>
#include <Sound/AudioSystem.h>

namespace Funkin
{
	class Conductor;

	class MainMenuSystem : public Stratum::ISceneSystem
	{
	public:

		MainMenuSystem();
		~MainMenuSystem() override;

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;

		Stratum::Ref<Stratum::MP3AudioSource> MusicSource;
		Stratum::Ref<Stratum::MP3AudioSource> ConfirmFxSource;
		Stratum::Ref<Stratum::MP3AudioSource> CancelFxSource;

		void CreateControllerPrompt(GamepadButton button, const std::wstring& text);
		void ClearControllerPrompts();

	private:

		Stratum::Scene* mScene;
	};
}