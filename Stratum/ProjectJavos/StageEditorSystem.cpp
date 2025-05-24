#include "StageEditorSystem.h"

#include "StageInfo.h"

#include <Thirdparty/imgui/imgui.h>

#include <Core/EngineStats.h>
#include <Core/Time.h>

#include <Util/Globals.h>

#include <Renderer/RendererContext.h>
#include <Renderer/ImageResource.h>

#include <json/json.hpp>

#undef min

struct PropMetadata
{
	std::string assetPath;
	glm::vec2 Scroll;
	float DanceEvery;
	bool _UpdateProp = false;
};

Javos::StageEditorSystem::StageEditorSystem(const std::string& stage)
{
}

Javos::StageEditorSystem::~StageEditorSystem()
{
}

void Javos::StageEditorSystem::Init(Stratum::Scene* scene)
{
	mScene = scene;

	mScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<PropMetadata>(), "prop_metadata");

	auto metadataManager = mScene->GetComponentManager<PropMetadata>("prop_metadata");

	const char* names[] =
	{
		"prop01",
		"prop02",
		"prop03",
		"prop04",
	};
	for (int i = 0; i < 4; i++)
	{
		auto entity = mScene->EntityManager.CreateEntity();
		auto& sprite = mScene->SpriteRenderers.Create(entity);
		auto& transform = mScene->Transforms.Create(entity);
		auto& name = mScene->Names.Create(entity);
		metadataManager->Create(entity);
		name.Name = names[i];
	}
}

void Javos::StageEditorSystem::Update(Stratum::Scene* scene)
{

}

void Javos::StageEditorSystem::PostUpdate(Stratum::Scene* scene)
{

}

void Javos::StageEditorSystem::RenderImGui(Stratum::Scene* scene)
{
	using namespace Stratum;

	static int frameRate = 0;

	frameRate = (frameRate + (int)(1.0f / gpGlobals->deltaTime)) / 2;

	ImGui::Begin("EngineStats");

	float dtms = gpGlobals->deltaTime * 1000.0f;
	float gpms = Time::GPUTime.load() * 1000.0f;

	int gpuUsage = glm::min((int)((gpms / dtms) * 100.0f), 100);

	ImGui::Text("Frametime: %.2fms, GPU: %.2fms Usage: %i%%, FPS: %i", dtms, gpms, gpuUsage, frameRate);
	ImGui::Text("Vram: %.2fmb", (float)Render::RendererContext::s_Context->GetGraphicsDeviceProperties().UsedVideoMemory / 1024.0f / 1024.0f);
	ImGui::Text("ECS Stats [Live/Max] %i/%i", mScene->EntityManager.LiveEntities, mScene->EntityManager.MaxEntities);

	auto times = EngineStats::GetTimes();

	for (auto& t : times)
	{
		ImGui::Text("%s: %.2fms", t.name, t.time);
	}

	ImGui::End();

	DrawProps();
	DrawPropManager();

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

void Javos::StageEditorSystem::DrawProps()
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

		if (metadata._UpdateProp)
		{
			if (Stratum::ZVFS::Exists(metadata.assetPath.c_str()))
			{
				sprite.TextureHandle = mScene->Resources.LoadTextureImage(metadata.assetPath);
				if (sprite.Rect.size.x == 0 && sprite.Rect.size.y == 0)
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
			
			name.Name = "Empty Prop";

		}

		ImGui::EndPopup();
	}

	ImGui::End();
}

void Javos::StageEditorSystem::DrawPropManager()
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
					sprite.Rect.size = {};
				}
			}

			ImGui::End();
		}

		ImGui::DragFloat2("Position", glm::value_ptr(transform.Position));
		ImGui::DragFloat2("Scale", glm::value_ptr(transform.Scale), 0.1f);
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

void Javos::StageEditorSystem::SaveJson()
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
		prop.zIndex = sprite.RenderLayer;
		prop.Scroll = {};
		prop.Name = name.Name;

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

		propArray.push_back(propJson);

	}

	json["cameraZoom"] = stageRoot.CameraZoom;
	json["name"] = stageRoot.Name;
	json["props"] = propArray;

	std::string dump = json.dump();

	std::ofstream output(path.c_str());
	output << dump;
	output.close();

}

void Javos::StageEditorSystem::ReadJson(const std::string& name)
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
		transform.Position.x = prop["position"][0];
		transform.Position.y = prop["position"][1];
		transform.Scale.x = prop["scale"][0];
		transform.Scale.y = prop["scale"][1];
		sprite.RenderLayer = prop["zIndex"];
	}
}
