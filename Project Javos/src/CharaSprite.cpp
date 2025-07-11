#include "CharaSprite.h"

#include "Conductor.h"
#include "GameState.h"
#include "SparrowReader.h"

#include <Scene/Scene.h>
#include <VFS/ZVFS.h>
#include <Util/Globals.h>
#include <json/json.hpp>

extern Funkin::GameState gGameState;

Funkin::CharaSprite::CharaSprite(Stratum::Scene* scene, const std::string& file)
{
	mScene = scene;
	CharaEntity = Stratum::ECS::C_INVALID_ENTITY;

	if (!Stratum::ZVFS::Exists(file.c_str()))
		return;

	auto entity = mScene->EntityManager.CreateEntity();

	auto& renderer = mScene->SpriteRenderers.Create(entity);
	auto& transform = mScene->Transforms.Create(entity);
	auto& animator = mScene->SpriteAnimators.Create(entity);

	renderer.Center = glm::vec2(0.0f, 1.0f);

	CharaEntity = entity;

	nlohmann::json json = nlohmann::json::parse(Stratum::ZVFS::GetFile(file.c_str())->Str());

	std::string assetPath = json["assetPath"];
	std::string sparrowPath = json["sparrowPath"];

	if (json.contains("scale"))
	{
		mOriginalScale.x = json["scale"][0];
		mOriginalScale.y = json["scale"][1];
	}
	else
	{
		mOriginalScale = glm::vec2(1.0f);
	}

	for (auto& anim : json["animations"])
	{
		std::string prefix = anim["prefix"];
		std::string name = anim["name"];
		bool isIdle = name.compare("idle") == 0;

		float mult = isIdle ? gGameState.pConductor->GetConductorBeatMultiplier() * 0.5f : 1.0f;
		float duration = isIdle ? 1.0f : (float)anim["duration"];

		auto frames = SparrowReader::readXML(sparrowPath, prefix, !isIdle);

		Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(frames.size() * mult * duration)
			.SetLoop(isIdle)
			.SetAnimateOnIdle(isIdle)
			.SetFrames(frames);

		animator.AnimationMap[name] = animation;
		mOffsetMap[name] = glm::vec2{ anim["offsets"][0], anim["offsets"][1] };
	}

	mOffsetMap["idle"] = glm::vec2{ 0.0f };

	animator.DefaultAnimation = "idle";
	animator.SetState("idle");

	if (json.contains("flipX"))
	{
		renderer.FlipX = json["flipX"];
		mIsFlipped = renderer.FlipX;
	}

	renderer.TextureHandle = mScene->Resources.LoadTextureImage(assetPath);

	if (json.contains("usePixel"))
		renderer.UseNearestTextureFilter = json["usePixel"];
	
	SetEnabled(false);

	mBeatAcumulator = 0.0f;
	mSparrowPath = sparrowPath;
}

void Funkin::CharaSprite::Update()
{
	auto& animator = mScene->SpriteAnimators.Get(CharaEntity);
	auto& transform = mScene->Transforms.Get(CharaEntity);
	auto& renderer = mScene->SpriteRenderers.Get(CharaEntity);

	uint32_t doDanceEvery = glm::max(gGameState.DoBeatEveryNthBeat, 2U);

	if ((gGameState.pConductor->BeatCount + gGameState.BeatOffset) % doDanceEvery == 0)
	{
		mDoBeat = true;
	}

	size_t frameCount = animator.AnimationMap["idle"].rects.size();

	float bpmIdleFps = 1.0f / (frameCount * (gGameState.pConductor->chart.info.bpm / 2.0f) / 60.0f);
	mBeatAcumulator += Stratum::gpGlobals->deltaTime;

	while (mBeatAcumulator >= bpmIdleFps)
	{
		mFrameIndex += 1;
		mBeatAcumulator -= bpmIdleFps;
	}
	if (mFrameIndex > frameCount - 1)
	{
		mDoBeat = false;
	}

	uint32_t frameIndex = mFrameIndex;

	if (!mDoBeat)
	{
		mFrameIndex = 0;
		frameIndex = frameCount - 1;
	}

	animator.AnimationMap["idle"].FrameIndex = frameIndex;

	UpdateTransform();
}

void Funkin::CharaSprite::UpdateTransform()
{
	auto& animator = mScene->SpriteAnimators.Get(CharaEntity);
	auto& transform = mScene->Transforms.Get(CharaEntity);
	auto& renderer = mScene->SpriteRenderers.Get(CharaEntity);

	glm::vec2 multiplier = { renderer.FlipX ? -1.0f : 1.0f, 1.0f };
	glm::vec2 targetScale = mOriginalScale * CharaScale;

	glm::vec3 Position = glm::vec3(CharaPosition + (mOffsetMap[animator.CurrentAnimation] * CharaScale) * multiplier, 0.0f);
	transform.SetPosition(Position);
	transform.SetScale(glm::vec3(targetScale, 1.0f));
}

void Funkin::CharaSprite::AddAnimation(const std::string& name, const std::string& prefix, const std::string& nextState, float fps, bool loop, bool alwaysSync)
{
	auto frames = SparrowReader::readXML(mSparrowPath, prefix, false);

	Stratum::SpriteAnimator::Animation animation = Stratum::SpriteAnimator::Animation()
		.SetFrameRate(fps)
		.SetLoop(loop)
		.SetAnimateOnIdle(alwaysSync)
		.SetNextState(nextState)
		.SetFrames(frames);

	if (!loop)
	{
		if (alwaysSync)
		{
			animation.SetTransitionToDefault(false);
		}
	}

	auto& animator = mScene->SpriteAnimators.Get(CharaEntity);
	animator.AnimationMap[name] = animation;
}

void Funkin::CharaSprite::PlayAnimation(const std::string& name)
{
	auto& animator = mScene->SpriteAnimators.Get(CharaEntity);
	auto& renderer = mScene->SpriteRenderers.Get(CharaEntity);

	std::string newName = name;

	if (renderer.FlipX && name.compare("left") == 0)
		newName = "right";

	if (renderer.FlipX && name.compare("right") == 0)
		newName = "left";

	if (!animator.AnimationMap.contains(newName))
		return;

	if (animator.CurrentAnimation.compare(newName) == 0)
	{
		if (animator.AnimationMap[animator.CurrentAnimation].FrameIndex >= 2)
		{
			animator.SetState(newName);
		}
	}
	else
	{
		animator.SetState(newName);
	}
}

void Funkin::CharaSprite::SetEnabled(bool enabled)
{
	auto& renderer = mScene->SpriteRenderers.Get(CharaEntity);
	renderer.Enabled = enabled;

	if (enabled)
	{
		auto& animator = mScene->SpriteAnimators.Get(CharaEntity);
		animator.SetState("idle");
	}
}

void Funkin::CharaSprite::SetCenter(const glm::vec2& center)
{
	auto& renderer = mScene->SpriteRenderers.Get(CharaEntity);
	renderer.Center = center;
}

void Funkin::CharaSprite::SetLayer(uint32_t layer)
{
	auto& renderer = mScene->SpriteRenderers.Get(CharaEntity);
	renderer.RenderLayer = layer;
}

bool Funkin::CharaSprite::FlippedHorizontally()
{
	return mIsFlipped;
}
