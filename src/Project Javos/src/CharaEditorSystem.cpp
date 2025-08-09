#include "CharaEditorSystem.h"
#include "SparrowReader.h"
#include "InGameSystem.h"

#include <Thirdparty/imgui/imgui.h>
#include <Input/Input.h>
#include <json/json.hpp>

Funkin::CharaEditorSystem::CharaEditorSystem(const std::string& stage)
{
	mLoadedWith = stage;
}

Funkin::CharaEditorSystem::~CharaEditorSystem()
{
}

void Funkin::CharaEditorSystem::Init(Stratum::Scene* scene)
{
	mScene = scene;

	mCharacterEntity = mScene->EntityManager.CreateEntity();
	mIdleEntity = mScene->EntityManager.CreateEntity();

	mScene->SpriteRenderers.Create(mIdleEntity);
	mScene->SpriteAnimators.Create(mIdleEntity);
	mScene->Transforms.Create(mIdleEntity);

	mScene->SpriteRenderers.Create(mCharacterEntity);
	mScene->SpriteAnimators.Create(mCharacterEntity);
	mScene->Transforms.Create(mCharacterEntity);

	mCurrentState = "idle";
}

void Funkin::CharaEditorSystem::Update(Stratum::Scene* scene)
{

	if (Stratum::Input::GetKeyDown(KeyCode::ESCAPE))
	{
		LoadChartParams params;
		params.ChartPath = mLoadedWith;

		if (!mCharaName.empty())
			params.OverridePlayer1 = mCharaName;

		auto scene = new Stratum::Scene();
		scene->RegisterCustomSystem(new InGameSystem(params));
		mScene->SwapScene(scene);
	}

	auto& transform = mScene->Transforms.Get(mCharacterEntity);
	auto& sprite = mScene->SpriteRenderers.Get(mCharacterEntity);
	auto& animator = mScene->SpriteAnimators.Get(mCharacterEntity);

	auto& transformGhost = mScene->Transforms.Get(mIdleEntity);
	auto& spriteGhost = mScene->SpriteRenderers.Get(mIdleEntity);
	auto& animatorGhost = mScene->SpriteAnimators.Get(mIdleEntity);

	animator.DefaultAnimation = "idle";

	spriteGhost.SpriteColor.a = 0.5f;
	spriteGhost.Center = glm::vec2(0.0f, 1.0f);
	spriteGhost.Enabled = mShowIdle;
	spriteGhost.TextureHandle = sprite.TextureHandle;

	animatorGhost.AnimationMap = animator.AnimationMap;
	animatorGhost.SetState("idle");

	sprite.SpriteColor.a = mShowIdle ? 0.6f : 1.0f;
	sprite.Center = glm::vec2(0.0f, 1.0f);

	bool set = false;

	if (animator.AnimationMap.contains("left"))
		animator.AnimationMap["left"].SetFrameRate(animator.AnimationMap["left"].rects.size() * mLeftAnimation.Duration);
	if (animator.AnimationMap.contains("right"))
		animator.AnimationMap["right"].SetFrameRate(animator.AnimationMap["right"].rects.size() * mLeftAnimation.Duration);
	if (animator.AnimationMap.contains("up"))
		animator.AnimationMap["up"].SetFrameRate(animator.AnimationMap["up"].rects.size() * mLeftAnimation.Duration);
	if (animator.AnimationMap.contains("down"))
		animator.AnimationMap["down"].SetFrameRate(animator.AnimationMap["down"].rects.size() * mLeftAnimation.Duration);

	if (Stratum::Input::GetKeyDown(KeyCode::LEFT))
	{
		mCurrentState = "left";
		set = true;
	}
	if (Stratum::Input::GetKeyDown(KeyCode::UP))
	{
		mCurrentState = "up";
		set = true;
	}
	if (Stratum::Input::GetKeyDown(KeyCode::DOWN))
	{
		mCurrentState = "down";
		set = true;
	}
	if (Stratum::Input::GetKeyDown(KeyCode::RIGHT))
	{
		mCurrentState = "right";
		set = true;
	}
	if (Stratum::Input::GetKeyDown(KeyCode::SPACE))
	{
		mCurrentState = "idle";
		set = true;
	}

	if (mBackToIdle)
	{
		if (set)
		{
			animator.SetState(mCurrentState);
		}
	}
	else
	{
		animator.SetState(mCurrentState);
	}

	glm::vec2 multiplier = { sprite.FlipX ? -1.0f : 1.0f, 1.0f };
	glm::vec3 Position = glm::vec3(0.0f, -500.0f, 0.0f);

	transformGhost.SetPosition(Position);
	transformGhost.SetScale(transform.Scale);

	if (animator.CurrentAnimation.compare("left") == 0)
	{
		Position += glm::vec3(mLeftAnimation.Offset * multiplier, 0.0f);
	}
	if (animator.CurrentAnimation.compare("right") == 0)
	{
		Position += glm::vec3(mRightAnimation.Offset * multiplier, 0.0f);
	}
	if (animator.CurrentAnimation.compare("up") == 0)
	{
		Position += glm::vec3(mUpAnimation.Offset * multiplier, 0.0f);
	}
	if (animator.CurrentAnimation.compare("down") == 0)
	{
		Position += glm::vec3(mDownAnimation.Offset * multiplier, 0.0f);
	}

	transform.SetPosition(Position);
}

void Funkin::CharaEditorSystem::PostUpdate(Stratum::Scene* scene)
{
}

void Funkin::CharaEditorSystem::RenderImGui(Stratum::Scene* scene)
{
	EditCharacterGUI();

	static bool saveDialogOpen = false;
	static bool loadDialogOpen = false;

	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Save")) {
				saveDialogOpen = true;
			}
			if (ImGui::MenuItem("Load")) {
				loadDialogOpen = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (saveDialogOpen && ImGui::Begin("Save", &saveDialogOpen, ImGuiWindowFlags_AlwaysAutoResize))
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
			saveDialogOpen = false;

		}

		ImGui::End();
	}

	if (loadDialogOpen && ImGui::Begin("Load", &loadDialogOpen, ImGuiWindowFlags_AlwaysAutoResize))
	{

		auto textures = Stratum::ZVFS::GetAllOf("json");

		for (auto& str : textures)
		{
			if (str.find("characters") == std::string::npos)
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

void Funkin::CharaEditorSystem::EditCharacterGUI()
{
	auto& sprite = mScene->SpriteRenderers.Get(mCharacterEntity);
	auto& animator = mScene->SpriteAnimators.Get(mCharacterEntity);
	auto& transform = mScene->Transforms.Get(mCharacterEntity);

	ImGui::Begin("Character Editor");

	static bool isSelectWindowOpen = false;

	InputText(mCharaName, "Character Name");
	ImGui::Checkbox("Return to idle", &mBackToIdle);
	ImGui::Checkbox("Horizontal Flip", &sprite.FlipX);
	ImGui::Checkbox("Use pixel", &sprite.UseNearestTextureFilter);

	ImGui::DragFloat2("Scale", glm::value_ptr(transform.Scale), 0.025f);

	ImGui::Text("Asset Path: %s", mAssetPath.c_str());
	ImGui::Text("Sparrow Path: %s", mSparrowPath.c_str());
	if (ImGui::Button("Change"))
	{
		isSelectWindowOpen = true;
	}

	InputText(mIdleAnimation.Name, "Idle Animation");
	ImGui::Checkbox("Show idle animation", &mShowIdle);

	InputText(mLeftAnimation.Name, "Left Animation");
	ImGui::DragFloat2("Offset Left", glm::value_ptr(mLeftAnimation.Offset), 0.5f);
	ImGui::InputFloat("Left Multiplier", &mLeftAnimation.Duration);
	ImGui::Checkbox("Ignore Offset Left", &mLeftAnimation.IgnoreOffset);

	InputText(mDownAnimation.Name, "Down Animation");
	ImGui::DragFloat2("Offset Down", glm::value_ptr(mDownAnimation.Offset), 0.5f);
	ImGui::InputFloat("Down Multiplier", &mDownAnimation.Duration);
	ImGui::Checkbox("Ignore Offset Down", &mDownAnimation.IgnoreOffset);

	InputText(mUpAnimation.Name, "Up Animation");
	ImGui::DragFloat2("Offset Up", glm::value_ptr(mUpAnimation.Offset), 0.5f);
	ImGui::InputFloat("Up Multiplier", &mUpAnimation.Duration);
	ImGui::Checkbox("Ignore Offset Up", &mUpAnimation.IgnoreOffset);

	InputText(mRightAnimation.Name, "Right Animation");
	ImGui::DragFloat2("Offset Right", glm::value_ptr(mRightAnimation.Offset), 0.5f);
	ImGui::InputFloat("Right Multiplier", &mRightAnimation.Duration);
	ImGui::Checkbox("Ignore Offset Right", &mRightAnimation.IgnoreOffset);

	sprite.Center = glm::vec2(0.0f, 1.0f);

	if (isSelectWindowOpen && ImGui::Begin("Select Texture", &isSelectWindowOpen))
	{
		auto textures = Stratum::ZVFS::GetAllOf("png");
		auto texturesdds = Stratum::ZVFS::GetAllOf("dds");

		for (auto str : texturesdds)
		{
			textures.push_back(str);
		}

		for (auto& str : textures)
		{
			if (str.find("chara") == std::string::npos)
			{
				continue;
			}
			if (ImGui::Button(str.c_str()))
			{
				mAssetPath = str;
				std::string file = str.substr(0, str.size() - 3).append("xml");
				mSparrowPath = file;
				isSelectWindowOpen = false;
			}
		}

		ImGui::End();
	}

	if (ImGui::Button("Apply Animations") && !mSparrowPath.empty())
	{
		if (Stratum::ZVFS::Exists(mSparrowPath.c_str()))
		{
			Stratum::SpriteAnimator::Animation idleAnimation = Stratum::SpriteAnimator::Animation()
				.SetFrameRate(24)
				.SetLoop(true)
				.SetAnimateOnIdle(true)
				.SetFrames(SparrowReader::readXML(mSparrowPath, mIdleAnimation.Name, false));

			Stratum::SpriteAnimator::Animation leftAnimation = Stratum::SpriteAnimator::Animation()
				.SetFrameRate(24)
				.SetLoop(false)
				.SetAnimateOnIdle(false)
				.SetFrames(SparrowReader::readXML(mSparrowPath, mLeftAnimation.Name, mLeftAnimation.IgnoreOffset));

			Stratum::SpriteAnimator::Animation rightAnimation = Stratum::SpriteAnimator::Animation()
				.SetFrameRate(24)
				.SetLoop(false)
				.SetAnimateOnIdle(false)
				.SetFrames(SparrowReader::readXML(mSparrowPath, mRightAnimation.Name, mRightAnimation.IgnoreOffset));

			Stratum::SpriteAnimator::Animation upAnimation = Stratum::SpriteAnimator::Animation()
				.SetFrameRate(24)
				.SetLoop(false)
				.SetAnimateOnIdle(false)
				.SetFrames(SparrowReader::readXML(mSparrowPath, mUpAnimation.Name, mUpAnimation.IgnoreOffset));

			Stratum::SpriteAnimator::Animation downAnimation = Stratum::SpriteAnimator::Animation()
				.SetFrameRate(24)
				.SetLoop(false)
				.SetAnimateOnIdle(false)
				.SetFrames(SparrowReader::readXML(mSparrowPath, mDownAnimation.Name, mDownAnimation.IgnoreOffset));

			downAnimation.SetFrameRate(downAnimation.rects.size() * mDownAnimation.Duration);
			upAnimation.SetFrameRate(upAnimation.rects.size() * mUpAnimation.Duration);
			rightAnimation.SetFrameRate(rightAnimation.rects.size() * mRightAnimation.Duration);
			leftAnimation.SetFrameRate(leftAnimation.rects.size() * mLeftAnimation.Duration);

			animator.AnimationMap["idle"] = idleAnimation;
			animator.AnimationMap["up"] = upAnimation;
			animator.AnimationMap["down"] = downAnimation;
			animator.AnimationMap["left"] = leftAnimation;
			animator.AnimationMap["right"] = rightAnimation;

			animator.SetState("idle");
		}
		if (Stratum::ZVFS::Exists(mAssetPath.c_str()))
		{
			sprite.TextureHandle = mScene->Resources.LoadTextureImage(mAssetPath);
		}
	}

	ImGui::End();
}

void Funkin::CharaEditorSystem::SaveJson()
{
	std::string path = "Data/fnf/characters/data/";
	path.append(mSaveOutput).append(".json");

	nlohmann::json json;
	auto& sprite = mScene->SpriteRenderers.Get(mCharacterEntity);
	auto& transform = mScene->Transforms.Get(mCharacterEntity);

	json["name"] = mCharaName;
	json["assetPath"] = mAssetPath;
	json["sparrowPath"] = mSparrowPath;
	json["flipX"] = sprite.FlipX;
	json["scale"][0] = transform.Scale.x;
	json["scale"][1] = transform.Scale.y;
	json["usePixel"] = sprite.UseNearestTextureFilter;
	
	CharaAnimation animations[5] =
	{
		mIdleAnimation,
		mLeftAnimation,
		mDownAnimation,
		mUpAnimation,
		mRightAnimation
	};

	const char* names[5] =
	{
		"idle",
		"left",
		"down",
		"up",
		"right"
	};

	for (int i = 0; i < 5; i++)
	{
		nlohmann::json anim;
		anim["name"] = names[i];
		anim["prefix"] = animations[i].Name;
		anim["duration"] = animations[i].Duration;
		anim["offsets"][0] = animations[i].Offset.x;
		anim["offsets"][1] = animations[i].Offset.y;
		anim["ignoreOffsets"][1] = animations[i].IgnoreOffset;
		json["animations"][i] = anim;
	}

	std::string dump = json.dump();

	std::ofstream output(path.c_str());
	output << dump;
	output.close();
}

void Funkin::CharaEditorSystem::ReadJson(const std::string& name)
{
	if (!Stratum::ZVFS::Exists(name.c_str()))
		return;

	nlohmann::json json = nlohmann::json::parse(Stratum::ZVFS::GetFile(name.c_str())->Str());

	auto& sprite = mScene->SpriteRenderers.Get(mCharacterEntity);
	auto& animator = mScene->SpriteAnimators.Get(mCharacterEntity);
	auto& transform = mScene->Transforms.Get(mCharacterEntity);

	CharaAnimation* animations[5] =
	{
		&mIdleAnimation,
		&mLeftAnimation,
		&mDownAnimation,
		&mUpAnimation,
		&mRightAnimation
	};

	bool b[5] =
	{
		true,
		false,
		false,
		false,
		false
	};

	mCharaName = json["name"];
	mAssetPath = json["assetPath"];
	mSparrowPath = json["sparrowPath"];

	if (json.contains("flipX"))
		sprite.FlipX = json["flipX"];

	if (json.contains("scale"))
	{
		transform.Scale.x = json["scale"][0];
		transform.Scale.y = json["scale"][1];
	}

	if (json.contains("usePixel"))
		sprite.UseNearestTextureFilter = json["usePixel"];

	sprite.TextureHandle = mScene->Resources.LoadTextureImage(mAssetPath);

	for (int i = 0; i < 5; i++)
	{
		auto& anim = json["animations"][i];

		animations[i]->Duration = anim["duration"];
		animations[i]->Name = anim["prefix"];
		animations[i]->Offset.x = anim["offsets"][0];
		animations[i]->Offset.y = anim["offsets"][1];
		if (anim.contains("animOffsets"))
			animations[i]->IgnoreOffset = anim["ignoreOffsets"];

		Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(24)
			.SetLoop(b[i])
			.SetAnimateOnIdle(b[i])
			.SetFrames(SparrowReader::readXML(mSparrowPath, animations[i]->Name, i != 0));

		std::string name = anim["name"];
		animator.AnimationMap[name] = animation;
	}
}

void Funkin::CharaEditorSystem::InputText(std::string& target, const char* label)
{
	char buffer[255]{};

	memcpy(buffer, target.data(), target.size());

	if (ImGui::InputText(label, buffer, sizeof(buffer)))
	{
		target = buffer;
	}
}
