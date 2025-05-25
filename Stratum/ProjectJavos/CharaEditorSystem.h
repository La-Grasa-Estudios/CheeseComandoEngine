#pragma once

#include "Common.h"

#include <Scene/Scene.h>
#include <Sound/AudioSystem.h>

namespace Funkin
{
	class Conductor;

	class CharaEditorSystem : public Stratum::ISceneSystem
	{
	public:

		CharaEditorSystem(const std::string& stage);
		~CharaEditorSystem();

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;

	private:

		std::string mLoadedWith;

		enum AnimationKind
		{
			IDLE,
			UP,
			DOWN,
			LEFT,
			RIGHT
		};

		struct CharaAnimation
		{
			std::string Name;
			glm::vec2 Offset;
			float Duration = 1.0f;
			bool IgnoreOffset = false;
		};

		void EditCharacterGUI();
		void LoadAnimationGUI();

		void SaveJson();
		void ReadJson(const std::string& name);	

		void InputText(std::string& target, const char* label);

		AnimationKind mCurrentAnimationKind = IDLE;

		Stratum::ECS::edict_t mCharacterEntity;
		Stratum::ECS::edict_t mIdleEntity;

		std::string mSaveOutput;
		std::string mLoadFileString;
		std::string mAssetPath;
		std::string mSparrowPath;
		std::string mCurrentState;
		std::string mCharaName;

		CharaAnimation mIdleAnimation;
		CharaAnimation mUpAnimation;
		CharaAnimation mDownAnimation;
		CharaAnimation mRightAnimation;
		CharaAnimation mLeftAnimation;

		bool mShowIdle = false;
		bool mBackToIdle = false;

		Stratum::Scene* mScene;
	};
}