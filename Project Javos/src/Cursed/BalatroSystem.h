#pragma once

#include <Scene/Scene.h>
#include <Renderer/ConstantBuffer.h>
#include <Renderer/GraphicsCommandBuffer.h>

#include <Sound/SngAudioSource.h>
#include <Sound/MP3AudioSource.h>

namespace Funkin
{
	static inline const char* C_CARD_COMPONENT = "CardComponent";
	static inline const char* C_TILT_COMPONENT = "TextTiltComponent";

	struct CardComponent
	{
		float tiltX = 0.0f;
		float tiltY = 0.0f;
		float tiltFactor = 10.0f;
		float rotation = 0.0f;
		float moveSpeed = 4.0f;
		bool grabbable = true;
		bool grabbed = false;
		int seed = 0;
		glm::vec3 position = {};
		Stratum::ECS::edict_t bgEntity;
		Stratum::ECS::edict_t bgShadowEntity;
	};

	struct TextTiltComponent
	{
		int seed = 0;
		bool credits = true;
	};

	class BalatroSystem : public Stratum::ISceneSystem
	{
	public:

		struct ActionParameters
		{
			float duration;
			float amplitude = 1.0f;
			ActionParameters() = default;
			ActionParameters(const float dur);
			ActionParameters(const float dur, const float amp);
		};

		enum class Easing
		{
			Linear,
			Random,
			SineIn,
			SineOut,
			SineInOut,
			ElasticIn,
			ElasticOut,
			ElasticInOut,
			BackIn,
			BackOut,
			BackInOut,
			Sine,
			Cosine,
		};

		BalatroSystem();
		~BalatroSystem();

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;
		void UpdateCards();

		void PushAction(float* dst, float targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing);
		void PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing);

		void PushAction(float* dst, float targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec2* dst, glm::vec2 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec3* dst, glm::vec3 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);
		void PushAction(glm::vec4* dst, glm::vec4 targetVal, ActionParameters params, Easing easing, std::function<void()> cb);

		void DissolveCard(Stratum::ECS::edict_t cardEntity);
		void DestroyCard(Stratum::ECS::edict_t cardEntity);
		Stratum::ECS::edict_t CreateCard(uint32_t cardX, uint32_t cardY);
		Stratum::ECS::edict_t CreateTextEntity(const std::wstring& defaultText, const glm::vec2& pos, float fontSize = 64.0f, bool isGui = false, uint32_t renderLayer = 0, float align = 0.0f);
		Stratum::ECS::edict_t CreateRectEntity(const glm::vec2& pos, const glm::ivec2& rectSize = { 1.0f, 1.0f }, const glm::vec2& center = { 0.0f, 0.0f }, bool isGui = false, uint32_t renderLayer = 0);

	private:

		struct Action
		{
			Easing easing;
			uint8_t count;
			ActionParameters params;
			float time;
			float* floatPtr;
			float targetFloat[4];
			float srcFloat[4];
			std::function<void()> cb;
		};

		Stratum::ECS::edict_t mBgEntity;
		Stratum::ECS::edict_t mLogoEntity;
		Stratum::ECS::edict_t mCardEntity;
		Stratum::ECS::edict_t mExitText;
		Stratum::ECS::edict_t mExitButton;

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

		Stratum::Scene* mScene;

		std::vector<Action> mActions;

		float mDissolveTime = 0.0f;
		float mNextCardTimer = 0.0f;

		bool mCanDissolve = false;
		bool mPlayMenu = false;
		bool mPlayedMenuIntro = false;
		bool mFirstMenuFrame = false;
		bool mFirstButtonFrame = false;

	};
}