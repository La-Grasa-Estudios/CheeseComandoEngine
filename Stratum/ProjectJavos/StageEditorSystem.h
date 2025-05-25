#pragma once

#include "Common.h"

#include <Scene/Scene.h>
#include <Sound/AudioSystem.h>

namespace Javos
{
	class Conductor;

	class StageEditorSystem : public Stratum::ISceneSystem
	{
	public:

		StageEditorSystem(const std::string& stage);
		~StageEditorSystem();

		void Init(Stratum::Scene* scene) final;
		void Update(Stratum::Scene* scene) final;
		void PostUpdate(Stratum::Scene* scene) final;
		void RenderImGui(Stratum::Scene* scene) final;

	private:

		void DrawProps();
		void DrawPropManager();
		void EditCharacter();

		void SaveJson();
		void ReadJson(const std::string& name);

		void CreateBf();

		std::vector<Stratum::ECS::edict_t> mProps;
		Stratum::ECS::edict_t mSelectedProp = Stratum::ECS::C_INVALID_ENTITY;

		Stratum::ECS::edict_t mBfEntity;
		Stratum::ECS::edict_t mGfEntity;
		Stratum::ECS::edict_t mOponentEntity;

		std::string mSaveOutput;
		std::string mLoadFileString;
		bool mSaveDialogOpen = false;
		bool mDeleteSelected = false;
		bool mPreviewCamera = false;

		glm::vec2 CameraOffsets[3] = {};

		Stratum::Scene* mScene;
	};
}