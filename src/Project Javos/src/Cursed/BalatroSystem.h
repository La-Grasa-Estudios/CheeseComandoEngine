#pragma once

#include <Scene/Scene.h>
#include <Renderer/ConstantBuffer.h>
#include <Renderer/GraphicsCommandBuffer.h>

#include <Sound/SngAudioSource.h>
#include <Sound/MP3AudioSource.h>

#include "../TimedActionSystem.h"
#include "Components.h"

namespace Funkin
{
	class BalatroSystem : public Stratum::ISceneSystem
	{
	public:

		BalatroSystem();
		~BalatroSystem();

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;
		void UpdateCards();
		void SortCards();
		void SolvePokerHandType(bool playCards = false);

		void DissolveCard(Stratum::ECS::edict_t cardEntity);
		void DestroyCard(Stratum::ECS::edict_t cardEntity);
		Stratum::ECS::edict_t CreateCard(CardType type, CardSuit suit);
		Stratum::ECS::edict_t CreatePlayingCard(CardType type, CardSuit suit);
		Stratum::ECS::edict_t CreateTextEntity(const std::wstring& defaultText, const glm::vec2& pos, float fontSize = 64.0f, bool isGui = false, uint32_t renderLayer = 0, float align = 0.0f);
		Stratum::ECS::edict_t CreateRectEntity(const glm::vec2& pos, const glm::ivec2& rectSize = { 1.0f, 1.0f }, const glm::vec2& center = { 0.0f, 0.0f }, bool isGui = false, uint32_t renderLayer = 0);

		TimedActionSystem* pTimedActionSystem;

	private:

		enum EventType
		{
			EVENT_SCORE_CARD,
			EVENT_DRAW_CARD,
			EVENT_DISSOLVE_CARD,
			EVENT_END_SCORING,
			EVENT_WAIT,
		};

		struct GameEvent
		{
			EventType Type;
			Stratum::ECS::edict_t Entity;
			float Duration;
			union
			{
				uint32_t drawCardIndex;
				float SoundPitch;
			};
		};

		Stratum::ECS::edict_t mBgEntity;
		Stratum::ECS::edict_t mLogoEntity;
		Stratum::ECS::edict_t mCardEntity;
		Stratum::ECS::edict_t mExitText;
		Stratum::ECS::edict_t mExitButton;

		Stratum::ECS::edict_t mChipsText;
		Stratum::ECS::edict_t mMultText;
		Stratum::ECS::edict_t mPokerHandText;

		Stratum::ECS::edict_t mCurrentGrab = 0;

		Stratum::Ref<Stratum::Render::ConstantBuffer> mPerFrameData;
		Stratum::Ref<Stratum::Render::GraphicsCommandBuffer> mCmdBuffer;
		Stratum::Ref<Stratum::Render::GraphicsPipeline> mBalatroBgShader;
		Stratum::Ref<Stratum::Render::GraphicsPipeline> mBalatroDissolveShader;

		Stratum::Ref<Stratum::SngAudioSource> mBalatroSoundtrack;
		Stratum::Ref<Stratum::MP3AudioSource> mBalatroWhoosh;
		Stratum::Ref<Stratum::MP3AudioSource> mBalatroCrumple;
		Stratum::Ref<Stratum::MP3AudioSource> mBalatroMagic;
		Stratum::Ref<Stratum::MP3AudioSource> mBalatroPick;
		Stratum::Ref<Stratum::MP3AudioSource> mBalatroChips;

		Stratum::Scene* mScene;

		float mDissolveTime = 0.0f;
		float mNextCardTimer = 0.0f;

		bool mCanDissolve = false;
		bool mPlayMenu = false;
		bool mPlayedMenuIntro = false;
		bool mFirstMenuFrame = false;
		bool mFirstButtonFrame = false;
		bool mIsPlayingHand = false;
		bool mProcessNextEvent = false;

		uint32_t mPlayedCardsCount = 0;

		std::vector<GameEvent> mEvents;

	};
}