#include "StageRegistry.h"

#include "Common.h"
#include "GameState.h"
#include "Conductor.h"
#include "Components.h"
#include "InGameSystem.h"

#include <Renderer/ImageResource.h>
#include <VFS/ZVFS.h>

#include <json/json.hpp>

extern Funkin::GameState gGameState;

void Funkin::StageRegistry::Init(Stratum::Scene* scene)
{
	sStages.clear();
	sCurrentStage = NULL;
	sScene = scene;
}

void Funkin::StageRegistry::AddStage(const std::string& name)
{
	std::string stagePath = GenerateAssetPath(C_STAGE_PATH_PREFIX, name, "json");

	if (!Stratum::ZVFS::Exists(stagePath.c_str()))
		return;

	nlohmann::json json = nlohmann::json::parse(Stratum::ZVFS::GetFile(stagePath.c_str())->Str());
	auto metadataManager = sScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);

	for (auto& prop : json["props"])
	{
		auto entity = sScene->EntityManager.CreateEntity();
		auto& sprite = sScene->SpriteRenderers.Create(entity);
		auto& transform = sScene->Transforms.Create(entity);
		auto& nameCompo = sScene->Names.Create(entity);
		auto& meta = metadataManager->Create(entity);

		std::string assetPath = prop["assetPath"];

		nameCompo.Name = prop["name"];
		if (!assetPath.empty())
		{
			sprite.TextureHandle = sScene->Resources.LoadTextureImage(assetPath);
			sprite.Rect.size = sScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
		}
		else
		{
			sprite.Rect.size = { 1, 1 };
		}
		sprite.SpriteColor.a = prop["opacity"];
		sprite.RenderLayer = prop["zIndex"];
		sprite.Enabled = false;
		meta.Position.x = prop["position"][0];
		meta.Position.y = prop["position"][1];
		meta.Scroll.x = prop["scroll"][0];
		meta.Scroll.y = prop["scroll"][1];
		meta.StageName = name;
		transform.Scale.x = prop["scale"][0];
		transform.Scale.y = prop["scale"][1];
		if (prop.contains("color"))
		{
			sprite.SpriteColor.r = prop["color"][0];
			sprite.SpriteColor.g = prop["color"][1];
			sprite.SpriteColor.b = prop["color"][2];
		}
		if (prop.contains("usePixel"))
			sprite.UseNearestTextureFilter = prop["usePixel"];
	}

	LevelStage stage{};

	stage.Name = name;

	if (json.contains("characters"))
	{
		auto& characters = json["characters"];

		if (characters.contains("bf")) 
		{
			StageCharacter bfChara{};
			auto& bf = characters["bf"];

			bfChara.zIndex = bf["zIndex"];
			bfChara.Position.x = bf["position"][0];
			bfChara.Position.y = bf["position"][1];
			bfChara.Scale.x = bf["scale"][0];
			bfChara.Scale.y = bf["scale"][1];
			bfChara.CameraOffset.x = bf["cameraOffset"][0];
			bfChara.CameraOffset.y = bf["cameraOffset"][1];

			stage.Player = bfChara;
		}

		if (characters.contains("dad")) 
		{
			StageCharacter opponentChara{};
			auto& bf = characters["dad"];

			opponentChara.zIndex = bf["zIndex"];
			opponentChara.Position.x = bf["position"][0];
			opponentChara.Position.y = bf["position"][1];
			opponentChara.Scale.x = bf["scale"][0];
			opponentChara.Scale.y = bf["scale"][1];
			opponentChara.CameraOffset.x = bf["cameraOffset"][0];
			opponentChara.CameraOffset.y = bf["cameraOffset"][1];

			stage.Oponent = opponentChara;
		}
	}

	sStages[name] = stage;
}

void Funkin::StageRegistry::SetStage(const std::string& name)
{
	if (!sStages.contains(name))
		return;

	if (sCurrentStage)
	{
		DisableStage(sCurrentStage);
	}

	sCurrentStage = &sStages[name];

	if (auto chara = gGameState.pInGame->GetPlayerCharacter())
	{
		chara->CharaPosition = sCurrentStage->Player.Position;
		chara->CharaScale = sCurrentStage->Player.Scale;

		auto& sprite = sScene->SpriteRenderers.Get(chara->CharaEntity);
		sprite.RenderLayer = sCurrentStage->Player.zIndex;
	}

	EnableStage(sCurrentStage);
}

Funkin::LevelStage* Funkin::StageRegistry::GetCurrentStage()
{
	return sCurrentStage;
}

void Funkin::StageRegistry::DisableStage(LevelStage* stage)
{
	auto metadataManager = sScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
	auto& entities = metadataManager->GetEntities();

	for (auto entity : entities)
	{
		auto& sprite = sScene->SpriteRenderers.Get(entity);
		auto& meta = metadataManager->Get(entity);

		if (meta.StageName.compare(stage->Name) == 0)
		{
			sprite.Enabled = false;
		}
	}
}

void Funkin::StageRegistry::EnableStage(LevelStage* stage)
{
	auto metadataManager = sScene->GetComponentManager<StagePropComponent>(C_STAGE_PROP_COMPONENT_NAME);
	auto& entities = metadataManager->GetEntities();

	for (auto entity : entities)
	{
		auto& sprite = sScene->SpriteRenderers.Get(entity);
		auto& meta = metadataManager->Get(entity);

		if (meta.StageName.compare(stage->Name) == 0)
		{
			sprite.Enabled = true;
		}
	}
}
