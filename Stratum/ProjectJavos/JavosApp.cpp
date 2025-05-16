#include "JavosApp.h"

#include "Input/Input.h"

#include "Sound/AudioSystem.h"
#include "Scene/Scene.h"

#include "Util/Globals.h"

#include <Thirdparty/imgui/imgui.h>
#include <Core/EngineStats.h>
#include <Core/Time.h>

#include "SparrowReader.h"
#include "Components.h"
#include "Conductor.h"

#undef min

Javos::JavosApp::JavosApp(Stratum::ApplicationInfo appInfo) : Stratum::Application(appInfo)
{
}

uint32_t bfFrameIndex = 0;
uint32_t conductorBeatCount = 0;

bool bfDoBeat = false;
float bfAccumulator = 0.0f;
float conductorAccumulator = 0.0f;

Stratum::Ref<Stratum::MP3AudioSource> instSource;
Stratum::Ref<Stratum::MP3AudioSource> voicesSource;

Javos::Conductor conductor;

Stratum::ECS::edict_t BoyfriendEntity;

void Javos::JavosApp::OnInit()
{
	auto gameScene = new Stratum::Scene();
	SetScene(gameScene);

	gameScene->RegisterCustomComponent(new Stratum::ECS::ComponentManager<NoteComponent>(), C_NOTE_COMPONENT_NAME);

	auto sprite = BoyfriendEntity = gameScene->EntityManager.CreateEntity();

	auto& renderer = gameScene->SpriteRenderers.Create(sprite);
	auto& transform = gameScene->Transforms.Create(sprite);

	transform.SetScale(glm::vec3(0.25f));

	renderer.Rect.position = glm::vec2(0.0f);
	renderer.Rect.size = glm::vec2(1024.0f);
	renderer.Center = glm::vec2(0.0f, 1.0f);
	renderer.RenderLayer = renderer.LAYER_BG2;

	renderer.TextureHandle = gameScene->Resources.LoadTextureImage("textures/BOYFRIEND.DDS");

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

	auto& animator = gameScene->SpriteAnimators.Create(sprite);
	animator.AnimationMap["idle"] = idleAnimation;
	animator.AnimationMap["left"] = leftAnimation;
	animator.AnimationMap["right"] = rightAnimation;
	animator.AnimationMap["up"] = upAnimation;
	animator.AnimationMap["down"] = downAnimation;

	animator.SetState("idle");

	conductor.LoadChart(gameScene, "fnf/data/bite/bite-fernan.json");

	std::string instPath = "fnf/songs/";
	std::string voicesPath = "fnf/songs/";
	instPath.append(conductor.chart.info.song).append("/Inst.mp3");
	voicesPath.append(conductor.chart.info.song).append("/Voices.mp3");

	instSource = Stratum::CreateRef<Stratum::MP3AudioSource>(instPath.c_str(), m_AudioEngine->GetEngine());
	m_AudioEngine->AddSource(instSource);
	instSource->Play();

	if (conductor.chart.info.needsVoices)
	{
		voicesSource = Stratum::CreateRef<Stratum::MP3AudioSource>(voicesPath.c_str(), m_AudioEngine->GetEngine());
		m_AudioEngine->AddSource(voicesSource);
		voicesSource->Play();
	}

}

void Javos::JavosApp::OnFrameUpdate()
{

	float bpmPerSecond = 1.0f / (210.0f / 60.0f);
	static float songDeltaTime = 0.0f;
	static float lastSongTime = 0.0f;
	static glm::vec3 bfPosition = glm::vec3(0.0f);

	songDeltaTime = Stratum::gpGlobals->deltaTime; // audioSource->PositionF() - lastSongTime;

	conductorBeatCount = glm::floor((210.0f / 60.0f) * instSource->PositionF());
	float conductorBeatF = (210.0f / 60.0f) * instSource->PositionF();

	conductor.SongTime = instSource->PositionF();
	conductor.Update(mCurrentScene);

	uint32_t doBopEveryNBeat = 4;

	auto& transform = mCurrentScene->Transforms.Get(BoyfriendEntity);
	auto& sprite = mCurrentScene->SpriteRenderers.Get(BoyfriendEntity);

	if (conductorBeatCount >= 325 && conductorBeatCount < 392)
	{
		doBopEveryNBeat = 2;
	}

	if (conductorBeatCount >= 654 && conductorBeatCount < 682)
	{
		doBopEveryNBeat = 99999999;
	}

	if (conductorBeatCount >= 682 && conductorBeatCount < 810)
	{
		doBopEveryNBeat = 2;
	}

	if (conductorBeatCount >= 339 && conductorBeatF < 340.2f)
	{
		auto scale = transform.Scale;
		scale.x += 3.0f * Stratum::gpGlobals->deltaTime;
		scale.y -= 0.1f * Stratum::gpGlobals->deltaTime;
		transform.SetScale(scale);
	}

	if (conductorBeatF >= 955.15f-4 && conductorBeatF < 958.51f - 4)
	{
		auto scale = transform.Scale;
		scale.x += 1.0f * Stratum::gpGlobals->deltaTime;
		transform.SetScale(scale);
	}

	if (conductorBeatCount == 959 - 4)
	{
		transform.SetScale(glm::vec3(0.25f));
	}

	if (conductorBeatF >= 963.9f - 4 && conductorBeatF < 964.5f - 4)
	{
		auto scale = transform.Scale;
		scale.x += 6.0f * Stratum::gpGlobals->deltaTime;
		transform.SetScale(scale);
	}

	if (conductorBeatCount == 966 - 4)
	{
		transform.SetScale(glm::vec3(0.25f));
	}

	if (conductorBeatF >= 967.47f - 4)
	{
		auto scale = transform.Scale;

		scale.x += 1.0f * Stratum::gpGlobals->deltaTime;

		bfPosition.y -= 70.0f * Stratum::gpGlobals->deltaTime;
		sprite.Rotation.x -= 15.0f * Stratum::gpGlobals->deltaTime;

		transform.SetScale(scale);
	}

	if (glm::abs(conductorBeatF - 340.5f) <= 0.2f)
	{
		transform.SetScale(glm::vec3(0.25f));
	}

	if (conductorBeatF <= 2.0f)
	{
		transform.SetScale(glm::vec3(0.25f));
	}

	if (conductorBeatCount == 355)
	{
		sprite.Center = glm::vec2(0.0f);
		transform.SetScale(glm::vec3(10.25f));
	}
	if (conductorBeatCount == 356)
	{
		sprite.Center = glm::vec2(0.0f, 1.0f);
		transform.SetScale(glm::vec3(0.25f));
	}

	auto& animator = mCurrentScene->SpriteAnimators.Get(BoyfriendEntity);

	if (conductorBeatCount % doBopEveryNBeat == 0)
	{
		bfDoBeat = true;
	}

	float bpmIdleFps = 1.0f / (15.0f * (210.0f / 2.0f) / 60.0f);
	bfAccumulator += songDeltaTime;

	while (bfAccumulator >= bpmIdleFps)
	{
		bfFrameIndex += 1;
		bfAccumulator -= bpmIdleFps;
	}
	if (bfFrameIndex > 14)
	{
		bfDoBeat = false;
	}

	uint32_t frameIndex = bfFrameIndex;

	if (!bfDoBeat)
	{
		bfFrameIndex = 0;
		frameIndex = 14;
	}

	animator.AnimationMap["idle"].FrameIndex = frameIndex;

	transform.SetPosition(bfPosition);

	if (animator.CurrentAnimation.compare("left") == 0)
	{
		transform.SetPosition(bfPosition + glm::vec3(-7, 0.0f, 0.0f));
	}

	if (animator.CurrentAnimation.compare("right") == 0)
	{
		transform.SetPosition(bfPosition + glm::vec3(20, 0.0f, 0.0f));
	}

	if (animator.CurrentAnimation.compare("down") == 0)
	{
		transform.SetPosition(bfPosition + glm::vec3(0.0f, -20.0f, 0.0f));
	}
}

void Javos::JavosApp::OnFrameRenderImGui()
{
	using namespace Stratum;

	static int frameRate = 0;

	frameRate = (frameRate + (int)(1.0f / gpGlobals->deltaTime)) / 2;

	ImGui::Begin("EngineStats");
	ImGui::Text("Beats: %i", conductorBeatCount);

	float dtms = gpGlobals->deltaTime * 1000.0f;
	float gpms = Time::GPUTime.load() * 1000.0f;

	int gpuUsage = glm::min((int)((gpms / dtms) * 100.0f), 100);

	ImGui::Text("Frametime: %.2fms, GPU: %.2fms Usage: %i%%, FPS: %i", dtms, gpms, gpuUsage, frameRate);

	auto times = EngineStats::GetTimes();

	for (auto& t : times)
	{
		ImGui::Text("%s: %.2fms", t.name, t.time);
	}

	ImGui::End();
}

void Javos::JavosApp::Cleanup()
{
	instSource->Stop();
	if (voicesSource)
		voicesSource->Stop();
	instSource = NULL;
	voicesSource = NULL;
}
