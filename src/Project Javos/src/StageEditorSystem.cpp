#include "StageEditorSystem.h"

#include "StageInfo.h"
#include "LoadingScreenSystem.h"
#include "SparrowReader.h"
#include "GameState.h"
#include "ChartLoader.h"

#include <Thirdparty/imgui/imgui.h>

#include <Core/EngineStats.h>
#include <Core/Time.h>

#include <Input/Input.h>

#include <Util/Globals.h>

#include <Renderer/RendererContext.h>
#include <Renderer/ImageResource.h>

#include <Scene/Renderer3D.h>
#include <Scene/Renderer2D.h>

#include <json/json.hpp>

#undef min

struct PropMetadata
{
	std::string assetPath;
	glm::vec2 Scroll;
	glm::vec2 Position;
	float DanceEvery;
	bool _UpdateProp = false;
};

static glm::vec2 cameraPosition = {};

Funkin::StageEditorSystem::StageEditorSystem(const std::string& stage)
{
	mLoadedWith = stage;
}

Funkin::StageEditorSystem::~StageEditorSystem()
{
	delete mCharacter;
	if (mOponentCharacter)
		delete mOponentCharacter;
}

void Funkin::StageEditorSystem::Init(Stratum::Scene* scene)
{
	mScene = scene;

	{
		auto entity = scene->EntityManager.CreateEntity();
		auto& camera = scene->Cameras.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		camera.RendersToGui = true;
		camera.Orthographic = true;
	}

	{
		auto entity = scene->EntityManager.CreateEntity();
		auto& camera = scene->Cameras.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		camera.RendersToGui = true;
		camera.Orthographic = true;
		camera.RenderLayer = 1;
	}

	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<PropMetadata>(), "prop_metadata");

	CreateBf();
}

void Funkin::StageEditorSystem::Update(Stratum::Scene* scene)
{
	if (Stratum::Input::GetKeyDown(KeyCode::ESCAPE))
	{
		LoadChartParams params;
		params.ChartPath = mLoadedWith;
		auto scene = new Stratum::Scene();
		scene->RegisterCustomSystem(new LoadingScreenSystem(params));
		mScene->SwapScene(scene);
	}

	mCharacter->UpdateTransform();
	mOponentCharacter->UpdateTransform();
}

void Funkin::StageEditorSystem::PostUpdate(Stratum::Scene* scene)
{

}

void Funkin::StageEditorSystem::RenderImGui(Stratum::Scene* scene)
{
	using namespace Stratum;

	static int frameRate = 0;

	frameRate = (frameRate + (int)(1.0f / gpGlobals->deltaTime)) / 2;

	DrawProps();
	DrawPropManager();
	EditCharacter();

	static bool loadDialogOpen = false;

	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Save")) {
				mSaveDialogOpen = true;
			}
			if (ImGui::MenuItem("Load")) {
				loadDialogOpen = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (mSaveDialogOpen && ImGui::Begin("Save", &mSaveDialogOpen, ImGuiWindowFlags_AlwaysAutoResize))
	{

		char buffer[255]{};

		memcpy(buffer, mSaveOutput.data(), mSaveOutput.size());

		if (ImGui::InputText("Name", buffer, sizeof(buffer)))
		{
			mSaveOutput = buffer;
		}

		if (ImGui::Button("Save") && !mSaveOutput.empty())
		{

			SaveJson();
			mSaveDialogOpen = false;

		}

		ImGui::End();
	}

	if (loadDialogOpen && ImGui::Begin("Load", &loadDialogOpen, ImGuiWindowFlags_AlwaysAutoResize))
	{

		auto textures = Stratum::ZVFS::GetAllOf("json");

		for (auto& str : textures)
		{
			if (str.find("stages") == std::string::npos)
			{
				continue;
			}
			if (ImGui::Button(str.c_str()))
			{
				mLoadFileString = str;
				ReadJson(mLoadFileString);
				loadDialogOpen = false;
			}
		}

		ImGui::End();
	}

}

void Funkin::StageEditorSystem::DrawProps()
{
	auto metadataManager = mScene->GetComponentManager<PropMetadata>("prop_metadata");

	auto& props = metadataManager->GetEntities();

	ImGui::Begin("Props");

	ImVec2 size = ImGui::GetWindowSize();
	ImGuiStyle& style = ImGui::GetStyle();

	for (auto entity : props)
	{

		auto& name = mScene->Names.Get(entity);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		ImVec4 buttonCol = style.Colors[ImGuiCol_WindowBg];

		if (mSelectedProp == entity) {
			buttonCol = style.Colors[ImGuiCol_ButtonActive];
		}

		ImGui::PushStyleColor(ImGuiCol_Button, buttonCol);

		ImGui::PushID(name.Name.c_str());

		if (ImGui::Button(name.Name.c_str(), ImVec2(size.x, 15)))
		{
			mSelectedProp = entity;
		}

		ImGui::PopID();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();

		auto& metadata = metadataManager->Get(entity);
		auto& sprite = mScene->SpriteRenderers.Get(entity);
		auto& transform = mScene->Transforms.Get(entity);

		glm::vec2 pos = metadata.Position;
		glm::vec2 scroll = metadata.Scroll;
		glm::vec2 targetPos = metadata.Position + cameraPosition * scroll;

		transform.Position = glm::vec3(targetPos, 0.0f);

		transform.IsDirty = true;

		if (metadata._UpdateProp)
		{
			if (Stratum::ZVFS::Exists(metadata.assetPath.c_str()))
			{
				sprite.TextureHandle = mScene->Resources.LoadTextureImage(metadata.assetPath);
				if (sprite.Rect.size.x <= 1 && sprite.Rect.size.y <= 1)
				{
					sprite.Rect.size = mScene->Resources.GetImageHandle(sprite.TextureHandle)->GetSize();
				}
			}
			metadata._UpdateProp = false;
		}
	}

	if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(1) && !ImGui::IsAnyItemHovered() && !ImGui::IsPopupOpen("HierarchyEntity")) {
		ImGui::OpenPopup("HierarchyPopup");
	}

	if (ImGui::BeginPopup("HierarchyPopup")) {

		if (ImGui::MenuItem("Create prop")) {

			auto entity = mScene->EntityManager.CreateEntity();
			auto& sprite = mScene->SpriteRenderers.Create(entity);
			auto& transform = mScene->Transforms.Create(entity);
			auto& name = mScene->Names.Create(entity);
			metadataManager->Create(entity);
			
			sprite.Rect.size = { 1, 1 };

			name.Name = "Empty Prop";

		}

		ImGui::EndPopup();
	}

	ImGui::End();
}

void Funkin::StageEditorSystem::DrawPropManager()
{
	auto metadataManager = mScene->GetComponentManager<PropMetadata>("prop_metadata");

	static bool isSelectWindowOpen = false;
	bool updateProp = false;

	if (mSelectedProp != Stratum::ECS::C_INVALID_ENTITY)
	{
		ImGui::Begin("PropManager");

		auto& metadata = metadataManager->Get(mSelectedProp);
		auto& sprite = mScene->SpriteRenderers.Get(mSelectedProp);
		auto& transform = mScene->Transforms.Get(mSelectedProp);
		auto& name = mScene->Names.Get(mSelectedProp);

		{
			char buffer[255]{};

			memcpy(buffer, name.Name.data(), name.Name.size());

			if (ImGui::InputText("Name", buffer, sizeof(buffer)))
			{
				name.Name = buffer;
			}

			if (name.Name.empty())
			{
				name.Name = "No name";
			}
		}

		ImGui::Text("Asset Path: %s", metadata.assetPath.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Change"))
		{
			isSelectWindowOpen = true;
		}

		if (isSelectWindowOpen && ImGui::Begin("Select Texture", &isSelectWindowOpen))
		{
			auto textures = Stratum::ZVFS::GetAllOf("png");
			auto textures1 = Stratum::ZVFS::GetAllOf("jpg");
			auto textures2 = Stratum::ZVFS::GetAllOf("jpeg");
			auto textures3 = Stratum::ZVFS::GetAllOf("dds");

			for (auto str : textures1) textures.push_back(str);
			for (auto str : textures2) textures.push_back(str);
			for (auto str : textures3) textures.push_back(str);

			for (auto& str : textures)
			{
				if (str.find("fnf") == std::string::npos)
				{
					continue;
				}
				if (ImGui::Button(str.c_str()))
				{
					metadata.assetPath = str;
					isSelectWindowOpen = false;
					updateProp = true;
					sprite.Rect.size = { 1, 1 };
				}
			}

			ImGui::End();
		}

		ImGui::Checkbox("Use pixel", &sprite.UseNearestTextureFilter);
		ImGui::DragFloat2("Position", glm::value_ptr(metadata.Position));
		ImGui::DragFloat2("Scale", glm::value_ptr(transform.Scale), 0.025f);
		ImGui::DragFloat2("Scroll", glm::value_ptr(metadata.Scroll), 0.025f);
		ImGui::DragFloat("Rotation", &sprite.Rotation.x, 0.5f);
		ImGui::InputInt("Z Index", &sprite.RenderLayer);
		ImGui::ColorPicker4("Color", glm::value_ptr(sprite.SpriteColor));

		transform.IsDirty = true;

		if (updateProp || ImGui::Button("Update Prop"))
		{
			metadata._UpdateProp = true;
		}

		if (ImGui::Button("Delete"))
		{
			mScene->EntityManager.DestroyEntity(mSelectedProp);
			mSelectedProp = Stratum::ECS::C_INVALID_ENTITY;
		}

		ImGui::End();
	}
	else
	{
		isSelectWindowOpen = false;
	}
}

void Funkin::StageEditorSystem::EditCharacter()
{
	auto character = mCharacter;

	static int characterIndex = 0;
	const char* names[3] =
	{
		"Player",
		"Oponent",
		"Gf"
	};

	CharaSprite* characters[3] =
	{
		mCharacter,
		mOponentCharacter,
		NULL
	};

	ImGui::Begin("Character Editor");

	int lastCharacterIndex = characterIndex;
	ImGui::InputInt("Character", &characterIndex);

	if (characterIndex < 0)
	{
		characterIndex = 0;
	}
	else if (characterIndex > 2)
	{
		characterIndex = 2;
	}

	character = characters[characterIndex];

	if (lastCharacterIndex != characterIndex)
	{
		auto lastCharacter = characters[lastCharacterIndex];
		if (lastCharacter)
			lastCharacter->SetEnabled(false);
		if (character)
			character->SetEnabled(true);
	}

	if (!character)
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Now editing: %s", names[characterIndex]);

	ImGui::Checkbox("Preview Camera", &mPreviewCamera);

	auto entity = character->CharaEntity;

	auto& transform = mScene->Transforms.Get(entity);
	auto& sprite = mScene->SpriteRenderers.Get(entity);

	ImGui::DragFloat2("Camera Offset", glm::value_ptr(CameraOffsets[characterIndex]));
	ImGui::DragFloat2("Position", glm::value_ptr(character->CharaPosition));
	ImGui::DragFloat2("Scale", glm::value_ptr(character->CharaScale), 0.025f);
	ImGui::InputInt("Z Index", &sprite.RenderLayer);

	transform.IsDirty = true;

	if (mPreviewCamera)
	{
		cameraPosition = glm::mix(cameraPosition, glm::vec2(transform.Position) + CameraOffsets[characterIndex], 3.0f * Stratum::gpGlobals->deltaTime);
	}
	else
	{
		cameraPosition = glm::mix(cameraPosition, glm::vec2(0.0f), 3.0f * Stratum::gpGlobals->deltaTime);
	}

	// mScene->RenderPath2D->SetCameraPosition(cameraPosition);

	ImGui::End();
}

void Funkin::StageEditorSystem::SaveJson()
{
	auto metadataManager = mScene->GetComponentManager<PropMetadata>("prop_metadata");
	
	std::string path = "Data/fnf/stages/";
	path.append(mSaveOutput).append(".json");

	LevelStage stageRoot{};

	stageRoot.CameraZoom = 1.0f;
	stageRoot.Name = "stage_test";
	
	auto& props = metadataManager->GetEntities();

	for (auto entity : props)
	{
		auto& metadata = metadataManager->Get(entity);
		auto& sprite = mScene->SpriteRenderers.Get(entity);
		auto& transform = mScene->Transforms.Get(entity);
		auto& name = mScene->Names.Get(entity);

		StageProp prop{};

		prop.Asset = metadata.assetPath;
		prop.Opacity = sprite.SpriteColor.a;
		prop.Scale = transform.Scale;
		prop.Position = transform.Position;
		prop.Color = sprite.SpriteColor;
		prop.zIndex = sprite.RenderLayer;
		prop.Scroll = metadata.Scroll;
		prop.Name = name.Name;
		prop.UsePixel = sprite.UseNearestTextureFilter;

		stageRoot.Props.push_back(prop);
	}

	nlohmann::json json;
	
	nlohmann::json::array_t propArray;

	for (auto prop : stageRoot.Props)
	{
		nlohmann::json propJson;

		propJson["zIndex"] = prop.zIndex;
		propJson["position"][0] = prop.Position.x;
		propJson["position"][1] = prop.Position.y;
		propJson["scale"][0] = prop.Scale.x;
		propJson["scale"][1] = prop.Scale.y;
		propJson["scroll"][0] = prop.Scroll.x;
		propJson["scroll"][1] = prop.Scroll.y;
		propJson["opacity"] = prop.Opacity;
		propJson["name"] = prop.Name;
		propJson["assetPath"] = prop.Asset;
		propJson["usePixel"] = prop.UsePixel;
		propJson["color"][0] = prop.Color.r;
		propJson["color"][1] = prop.Color.g;
		propJson["color"][2] = prop.Color.b;

		propArray.push_back(propJson);

	}

	nlohmann::json characters;
	nlohmann::json player;
	nlohmann::json dad;

	{
		auto& transform = mScene->Transforms.Get(mBfEntity);
		auto& sprite = mScene->SpriteRenderers.Get(mBfEntity);

		player["zIndex"] = sprite.RenderLayer;
		player["position"][0] = transform.Position.x;
		player["position"][1] = transform.Position.y;
		player["scale"][0] = mCharacter->CharaScale.x;
		player["scale"][1] = mCharacter->CharaScale.y;
		player["cameraOffset"][0] = CameraOffsets[0].x;
		player["cameraOffset"][1] = CameraOffsets[0].y;
	}

	if (mOponentCharacter) {
		auto& transform = mScene->Transforms.Get(mOponentCharacter->CharaEntity);
		auto& sprite = mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity);

		dad["zIndex"] = sprite.RenderLayer;
		dad["position"][0] = transform.Position.x;
		dad["position"][1] = transform.Position.y;
		dad["scale"][0] = mOponentCharacter->CharaScale.x;
		dad["scale"][1] = mOponentCharacter->CharaScale.y;
		dad["cameraOffset"][0] = CameraOffsets[1].x;
		dad["cameraOffset"][1] = CameraOffsets[1].y;
	}

	characters["bf"] = player;
	characters["dad"] = dad;

	json["characters"] = characters;
	json["cameraZoom"] = stageRoot.CameraZoom;
	json["name"] = stageRoot.Name;
	json["props"] = propArray;

	std::string dump = json.dump();

	std::ofstream output(path.c_str());
	output << dump;
	output.close();

}

void Funkin::StageEditorSystem::ReadJson(const std::string& name)
{

	if (!Stratum::ZVFS::Exists(name.c_str()))
		return;

	nlohmann::json json = nlohmann::json::parse(Stratum::ZVFS::GetFile(name.c_str())->Str());
	auto metadataManager = mScene->GetComponentManager<PropMetadata>("prop_metadata");

	std::vector<Stratum::ECS::edict_t> entities;

	auto& props = metadataManager->GetEntities();

	for (auto entity : props)
	{
		entities.push_back(entity);
	}

	for (auto ent : entities)
	{
		mScene->EntityManager.DestroyEntity(ent);
	}

	for (auto& prop : json["props"])
	{
		auto entity = mScene->EntityManager.CreateEntity();
		auto& sprite = mScene->SpriteRenderers.Create(entity);
		auto& transform = mScene->Transforms.Create(entity);
		auto& name = mScene->Names.Create(entity);
		auto& meta = metadataManager->Create(entity);

		meta._UpdateProp = true;
		meta.assetPath = prop["assetPath"];
		name.Name = prop["name"];
		sprite.SpriteColor.a = prop["opacity"];
		meta.Position.x = prop["position"][0];
		meta.Position.y = prop["position"][1];
		meta.Scroll.x = prop["scroll"][0];
		meta.Scroll.y = prop["scroll"][1];
		transform.Scale.x = prop["scale"][0];
		transform.Scale.y = prop["scale"][1];
		sprite.RenderLayer = prop["zIndex"];

		sprite.Rect.size = { 1,1 };

		if (prop.contains("color"))
		{
			sprite.SpriteColor.r = prop["color"][0];
			sprite.SpriteColor.g = prop["color"][1];
			sprite.SpriteColor.b = prop["color"][2];
		}

		if (prop.contains("usePixel"))
			sprite.UseNearestTextureFilter = prop["usePixel"];
	}

	if (json.contains("characters"))
	{
		auto& characters = json["characters"];

		{
			auto& bf = characters["bf"];
			auto& transform = mScene->Transforms.Get(mBfEntity);
			auto& sprite = mScene->SpriteRenderers.Get(mBfEntity);

			sprite.RenderLayer = bf["zIndex"];
			mCharacter->CharaPosition.x = bf["position"][0];
			mCharacter->CharaPosition.y = bf["position"][1];
			mCharacter->CharaScale.x = bf["scale"][0];
			mCharacter->CharaScale.y = bf["scale"][1];
			CameraOffsets[0].x = bf["cameraOffset"][0];
			CameraOffsets[0].y = bf["cameraOffset"][1];
		}

		if (mOponentCharacter && characters.contains("dad"))
		{
			auto& dad = characters["dad"];
			auto& transform = mScene->Transforms.Get(mOponentCharacter->CharaEntity);
			auto& sprite = mScene->SpriteRenderers.Get(mOponentCharacter->CharaEntity);

			sprite.RenderLayer = dad["zIndex"];
			mOponentCharacter->CharaPosition.x = dad["position"][0];
			mOponentCharacter->CharaPosition.y = dad["position"][1];
			mOponentCharacter->CharaScale.x = dad["scale"][0];
			mOponentCharacter->CharaScale.y = dad["scale"][1];
			CameraOffsets[1].x = dad["cameraOffset"][0];
			CameraOffsets[1].y = dad["cameraOffset"][1];
		}
	}

}

void Funkin::StageEditorSystem::CreateBf()
{
	auto chart = ChartLoader::LoadChart(mLoadedWith);

	mCharacter = new CharaSprite(mScene, GenerateAssetPath(C_CHARA_PATH_PREFIX, chart.info.player1, "json"));
	mCharacter->SetEnabled(true);

	mBfEntity = mCharacter->CharaEntity;

	auto& animator = mScene->SpriteAnimators.Get(mCharacter->CharaEntity);
	animator.AnimationMap["idle"].FrameRate = animator.AnimationMap["idle"].rects.size() * 2.0f;

	if (!chart.info.player2.empty())
	{
		mOponentCharacter = new CharaSprite(mScene, GenerateAssetPath(C_CHARA_PATH_PREFIX, chart.info.player2, "json"));
		mOponentCharacter->SetEnabled(false);
		auto& animatoro = mScene->SpriteAnimators.Get(mOponentCharacter->CharaEntity);
		animatoro.AnimationMap["idle"].FrameRate = animatoro.AnimationMap["idle"].rects.size() * 2.0f;
	}
}
