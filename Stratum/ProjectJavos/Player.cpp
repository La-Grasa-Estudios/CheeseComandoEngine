#include "Player.h"

#include "Conductor.h"
#include "SparrowReader.h"

#include <Util/Globals.h>
#include <Event/EventHandler.h>

Javos::PlayerSystem::PlayerSystem(Conductor* conductor)
{
	mConductor = conductor;
	mScene = nullptr;
}

void Javos::PlayerSystem::Init(Stratum::Scene* scene)
{
	mScene = scene;
	scene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<PlayerComponent>(), "player_component");

	Stratum::EventHandler::RegisterListener([this](void* sender, void** args, uint32_t argc) 
		{
			auto playerManager = mScene->GetComponentManager<PlayerComponent>("player_component");
			auto& entities = playerManager->GetEntities();
			uint32_t noteType = (uint32_t)args[0];

			for (auto edict : entities)
			{
				auto& animator = mScene->SpriteAnimators.Get(edict);
				const char* animations[4] =
				{
					"left",
					"down",
					"up",
					"right"
				};

				animator.SetState(animations[noteType]);
			}

		}, Stratum::EventHandler::GetEventID("hit_note"), true);
}

void Javos::PlayerSystem::Update(Stratum::Scene* scene)
{
	auto playerManager = scene->GetComponentManager<PlayerComponent>("player_component");
	auto& entities = playerManager->GetEntities();

	for (auto edict : entities)
	{
		auto& playerEntity = playerManager->Get(edict);

		uint32_t doBopEveryNBeat = 4;

		auto& transform = mScene->Transforms.Get(edict);
		auto& sprite = mScene->SpriteRenderers.Get(edict);

		if (mConductor->BeatCount >= 325 && mConductor->BeatCount < 392)
		{
			doBopEveryNBeat = 2;
		}

		if (mConductor->BeatCount >= 654 && mConductor->BeatCount < 682)
		{
			doBopEveryNBeat = 99999999;
		}

		if (mConductor->BeatCount >= 682 && mConductor->BeatCount < 810)
		{
			doBopEveryNBeat = 2;
		}

		if (mConductor->BeatCount >= 339 && mConductor->BeatCountF < 340.2f)
		{
			auto scale = transform.Scale;
			scale.x += 3.0f * Stratum::gpGlobals->deltaTime;
			scale.y -= 0.1f * Stratum::gpGlobals->deltaTime;
			transform.SetScale(scale);
		}

		if (mConductor->BeatCountF >= 955.15f - 4 && mConductor->BeatCountF < 958.51f - 4)
		{
			auto scale = transform.Scale;
			scale.x += 1.0f * Stratum::gpGlobals->deltaTime;
			transform.SetScale(scale);
		}

		if (mConductor->BeatCount == 959 - 4)
		{
			transform.SetScale(glm::vec3(0.25f));
		}

		if (mConductor->BeatCountF >= 963.9f - 4 && mConductor->BeatCountF < 964.5f - 4)
		{
			auto scale = transform.Scale;
			scale.x += 6.0f * Stratum::gpGlobals->deltaTime;
			transform.SetScale(scale);
		}

		if (mConductor->BeatCount == 966 - 4)
		{
			transform.SetScale(glm::vec3(0.25f));
		}

		if (mConductor->BeatCountF >= 967.47f - 4)
		{
			auto scale = transform.Scale;

			scale.x += 1.0f * Stratum::gpGlobals->deltaTime;

			playerEntity.Position.y -= 70.0f * Stratum::gpGlobals->deltaTime;
			sprite.Rotation.x -= 15.0f * Stratum::gpGlobals->deltaTime;

			transform.SetScale(scale);
		}

		if (glm::abs(mConductor->BeatCountF - 340.5f) <= 0.2f)
		{
			transform.SetScale(glm::vec3(0.25f));
		}

		if (mConductor->BeatCountF <= 2.0f)
		{
			transform.SetScale(glm::vec3(0.25f));
		}

		if (mConductor->BeatCount == 355)
		{
			sprite.Center = glm::vec2(0.0f);
			transform.SetScale(glm::vec3(10.25f));
		}
		if (mConductor->BeatCount == 356)
		{
			sprite.Center = glm::vec2(0.0f, 1.0f);
			transform.SetScale(glm::vec3(0.25f));
		}

		auto& animator = mScene->SpriteAnimators.Get(edict);

		if (mConductor->BeatCount % doBopEveryNBeat == 0)
		{
			playerEntity.doBeat = true;
		}

		float bpmIdleFps = 1.0f / (15.0f * (mConductor->chart.info.bpm / 2.0f) / 60.0f);
		playerEntity.beatAcumulator += Stratum::gpGlobals->deltaTime;

		while (playerEntity.beatAcumulator >= bpmIdleFps)
		{
			playerEntity.frameIndex += 1;
			playerEntity.beatAcumulator -= bpmIdleFps;
		}
		if (playerEntity.frameIndex > 14)
		{
			playerEntity.doBeat = false;
		}

		uint32_t frameIndex = playerEntity.frameIndex;

		if (!playerEntity.doBeat)
		{
			playerEntity.frameIndex = 0;
			frameIndex = 14;
		}

		animator.AnimationMap["idle"].FrameIndex = frameIndex;

		transform.SetPosition(playerEntity.Position);

		if (animator.CurrentAnimation.compare("left") == 0)
		{
			transform.SetPosition(playerEntity.Position + glm::vec3(-7, 0.0f, 0.0f));
		}

		if (animator.CurrentAnimation.compare("right") == 0)
		{
			transform.SetPosition(playerEntity.Position + glm::vec3(20, 0.0f, 0.0f));
		}

		if (animator.CurrentAnimation.compare("down") == 0)
		{
			transform.SetPosition(playerEntity.Position + glm::vec3(0.0f, -20.0f, 0.0f));
		}

	}

}

void Javos::PlayerSystem::PostUpdate(Stratum::Scene* scene)
{
}

Stratum::ECS::edict_t Javos::PlayerSystem::CreatePlayer()
{
	auto playerManager = mScene->GetComponentManager<PlayerComponent>("player_component");

	auto sprite = mScene->EntityManager.CreateEntity();

	auto& renderer = mScene->SpriteRenderers.Create(sprite);
	auto& transform = mScene->Transforms.Create(sprite);
	auto& player = playerManager->Create(sprite);

	player.bfEntity = sprite;

	transform.SetScale(glm::vec3(0.25f));

	renderer.Rect.position = glm::vec2(0.0f);
	renderer.Rect.size = glm::vec2(1024.0f);
	renderer.Center = glm::vec2(0.0f, 1.0f);
	renderer.RenderLayer = renderer.LAYER_BG2;

	renderer.TextureHandle = mScene->Resources.LoadTextureImage("textures/BOYFRIEND.DDS");

	Stratum::SpriteAnimator::Animation idleAnimation = Stratum::SpriteAnimator::Animation()
		.SetFrameRate(15)
		.SetLoop(true)
		.SetAnimateOnIdle(false)
		.SetFrames(SparrowReader::readXML("textures/BOYFRIEND.xml", "BF idle dance", false));

	Stratum::SpriteAnimator::Animation leftAnimation = Stratum::SpriteAnimator::Animation()
		.SetFrameRate(30)
		.SetLoop(false)
		.SetAnimateOnIdle(false)
		.SetNextState("idle")
		.SetFrames(SparrowReader::readXML("textures/BOYFRIEND.xml", "BF NOTE LEFT00", false));

	Stratum::SpriteAnimator::Animation rightAnimation = Stratum::SpriteAnimator::Animation()
		.SetFrameRate(15)
		.SetLoop(false)
		.SetAnimateOnIdle(false)
		.SetNextState("idle")
		.SetFrames(SparrowReader::readXML("textures/BOYFRIEND.xml", "BF NOTE RIGHT00", true));

	Stratum::SpriteAnimator::Animation upAnimation = Stratum::SpriteAnimator::Animation()
		.SetFrameRate(17)
		.SetLoop(false)
		.SetAnimateOnIdle(false)
		.SetNextState("idle")
		.SetFrames(SparrowReader::readXML("textures/BOYFRIEND.xml", "BF NOTE UP00", false));

	Stratum::SpriteAnimator::Animation downAnimation = Stratum::SpriteAnimator::Animation()
		.SetFrameRate(25)
		.SetLoop(false)
		.SetAnimateOnIdle(false)
		.SetNextState("idle")
		.SetFrames(SparrowReader::readXML("textures/BOYFRIEND.xml", "BF NOTE DOWN00", true));

	auto& animator = mScene->SpriteAnimators.Create(sprite);
	animator.AnimationMap["idle"] = idleAnimation;
	animator.AnimationMap["left"] = leftAnimation;
	animator.AnimationMap["right"] = rightAnimation;
	animator.AnimationMap["up"] = upAnimation;
	animator.AnimationMap["down"] = downAnimation;

	animator.SetState("idle");

	return sprite;
}
