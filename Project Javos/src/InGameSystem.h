#pragma once

#include "Common.h"
#include "CharaSprite.h"
#include "TimedActionSystem.h"

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
		bool IsPaused() const { return mIsPaused; }

		Stratum::ECS::edict_t CreateTextEntity(const std::wstring& defaultText, const glm::vec2& pos, float fontSize = 64.0f, bool isGui = false, uint32_t renderLayer = 0, float align = 0.0f);
		Stratum::ECS::edict_t CreateSpriteEntity(const::std::string& spritePath, const glm::vec2& pos, const glm::vec2& scale = { 1.0f, 1.0f }, bool isGui = false, uint32_t renderLayer = 0, bool flipX = false);
		Stratum::ECS::edict_t CreateRectEntity(const glm::vec2& pos, const glm::ivec2& rectSize = { 1.0f, 1.0f }, const glm::vec2& center = { 0.0f, 0.0f }, bool isGui = false, uint32_t renderLayer = 0);

		TimedActionSystem* pTimedActionSystem;

		float CameraZoomModifier = 1.0f;
		float GuiZoomModifier = 0.0f;
		bool TrackPlayersEnabled = true;

	private:

		void UpdateStage();

		Stratum::Ref<SongBase> mSong;

		LoadChartParams mLoadParams;
		Stratum::ECS::edict_t mPlayerSprite;

		Stratum::ECS::edict_t mWhiteSprite;
		float mFadeToWhiteTime = 0.0f;
		float mFadeToWhiteBaseTime = 0.0f;
		float mFadeToWhiteIntensity = 0.0f;

		Stratum::Ref<Stratum::MP3AudioSource> pauseSource;
		Stratum::Ref<Stratum::MP3AudioSource> scrollSource;
		Stratum::Ref<Stratum::MP3AudioSource> instSource;
		Stratum::Ref<Stratum::MP3AudioSource> voicesSource;
		Stratum::Ref<Stratum::MP3AudioSource> missSources[3];

		glm::vec2 CameraOffsets[2];

		Stratum::Scene* mScene;
		Conductor* mConductor;
		CharaSprite* mPlayerCharacter = NULL;
		CharaSprite* mOponentCharacter = NULL;

		bool mHasSongStarted = false;
		bool mIsPaused = false;

		std::atomic_uint mLoadingStage;
		std::atomic_bool mLoadingDone;

		Stratum::ECS::edict_t mResumeText;
		Stratum::ECS::edict_t mVolumeText;
		Stratum::ECS::edict_t mBotplayText;
		Stratum::ECS::edict_t mBalatroText;
		Stratum::ECS::edict_t mExitText;
		Stratum::ECS::edict_t mSelectText;

		int32_t mPauseUiButtonIndex = 0;

		float mVolume = 1.0f;
		float mWaitTimer = 0.0f;
	};
}