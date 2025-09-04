#include <Input/Input.h>

#include "Conductor.h"
#include "Components.h"
#include "ChartLoader.h"
#include "SparrowReader.h"
#include "Events.h"

#include <Core/Logger.h>
#include <Scene/Scene.h>
#include <Event/EventBus.h>
#include <Util/Globals.h>
#include <Util/StrUtil.h>

const float PRECISION = 64.0F;
const float DISPLACEMENT = -256.0F;
float STRUM_LINE_Y = 200;
const float SAFE_ZONE = 1.0f / 3.0f;

static const int SICK_TIME_WINDOW = 45;
static const int GOOD_TIME_WINDOW = 90;
static const int BAD_TIME_WINDOW = 130;

std::array<Stratum::ECS::edict_t, 4> noteButtons;

const int32_t NOTE_BUTTON_LAYER = 0;
const int32_t NOTE_HOLD_LAYER = 5;
const int32_t NOTE_LAYER = 10;
const int32_t NOTE_EFFECT_LAYER = 15;

Funkin::Conductor::Conductor()
{
	RegisterEventHandler("StSetBot", [this](ChartEvent& event)
		{
			EnableBot = event.castBoolean(event.Arg1);
		});
}

void Funkin::Conductor::Init(Stratum::Scene* scene)
{
	Stratum::Input::BindAlias("funkin_left", KeyCode::A);
	Stratum::Input::BindAlias("funkin_left", KeyCode::LEFT);
	Stratum::Input::BindAlias("funkin_left", GamepadButton::DPAD_LEFT);
	Stratum::Input::BindAlias("funkin_left", GamepadButton::X);
	Stratum::Input::BindAlias("funkin_right", KeyCode::D);
	Stratum::Input::BindAlias("funkin_right", KeyCode::RIGHT);
	Stratum::Input::BindAlias("funkin_right", GamepadButton::DPAD_RIGHT);
	Stratum::Input::BindAlias("funkin_right", GamepadButton::B);
	Stratum::Input::BindAlias("funkin_up", KeyCode::W);
	Stratum::Input::BindAlias("funkin_up", KeyCode::UP);
	Stratum::Input::BindAlias("funkin_up", GamepadButton::DPAD_UP);
	Stratum::Input::BindAlias("funkin_up", GamepadButton::Y);
	Stratum::Input::BindAlias("funkin_down", KeyCode::S);
	Stratum::Input::BindAlias("funkin_down", KeyCode::DOWN);
	Stratum::Input::BindAlias("funkin_down", GamepadButton::A);
	Stratum::Input::BindAlias("funkin_down", GamepadButton::DPAD_DOWN);

	Stratum::Input::BindAlias("funkin_up", GamepadButton::RIGHT_SHOULDER);
	Stratum::Input::BindAlias("funkin_down", GamepadButton::LEFT_SHOULDER);
	Stratum::Input::BindAxisToAlias("funkin_right", GamepadAxis::RIGHT_TRIGGER);
	Stratum::Input::BindAxisToAlias("funkin_left", GamepadAxis::LEFT_TRIGGER);

	// Precache everything needed
	scene->Resources.LoadTextureImage("textures/noteSplashes.png");

	std::array<Stratum::SpriteRendererComponent::SpriteRect, 4> rects =
	{
		Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(310, 235), glm::ivec2(153, 157) },
		Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(0, 235), glm::ivec2(157, 153) },
		Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(784, 232), glm::ivec2(157, 153) },
		Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(157, 235), glm::ivec2(153, 157) }
	};

	std::array<Stratum::SpriteAnimator::Animation, 4> defaultAnimations =
	{
		Stratum::SpriteAnimator::Animation().SetLoop(true).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "arrowLEFT00", true, true)),
		Stratum::SpriteAnimator::Animation().SetLoop(true).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "arrowDOWN00", true, true)),
		Stratum::SpriteAnimator::Animation().SetLoop(true).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "arrowUP00", true, true)),
		Stratum::SpriteAnimator::Animation().SetLoop(true).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "arrowRIGHT00", true, true)),
	};

	std::array<Stratum::SpriteAnimator::Animation, 4> holdAnimations =
	{
		Stratum::SpriteAnimator::Animation().SetTransitionToDefault(false).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "left press00", true, true)),
		Stratum::SpriteAnimator::Animation().SetTransitionToDefault(false).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "down press00", true, true)),
		Stratum::SpriteAnimator::Animation().SetTransitionToDefault(false).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "up press00", true, true)),
		Stratum::SpriteAnimator::Animation().SetTransitionToDefault(false).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "right press00", true, true)),
	};

	std::array<Stratum::SpriteAnimator::Animation, 4> hitAnimations =
	{
		Stratum::SpriteAnimator::Animation().SetFrameRate(24).SetNextState("default").SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "left confirm00", true, true)),
		Stratum::SpriteAnimator::Animation().SetFrameRate(24).SetNextState("default").SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "down confirm00", true, true)),
		Stratum::SpriteAnimator::Animation().SetFrameRate(24).SetNextState("default").SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "up confirm00", true, true)),
		Stratum::SpriteAnimator::Animation().SetFrameRate(24).SetNextState("default").SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "right confirm00", true, true)),
	};

	for (int i = 0; i < 4; i++)
	{
		auto entity = scene->EntityManager.CreateEntity();
		auto& sprite = scene->SpriteRenderers.Create(entity);
		auto& animator = scene->SpriteAnimators.Create(entity);
		auto& transform = scene->Transforms.Create(entity);
		auto& anchor = scene->GuiAnchors.Create(entity);

		anchor.AnchorPoint = Stratum::GuiAnchorPoint::TOP;

		sprite.RenderLayer = NOTE_BUTTON_LAYER;
		sprite.Rect = rects[i];
		sprite.TextureHandle = scene->Resources.LoadTextureImage("textures/NOTE_assets.DDS");
		sprite.CameraLayer = 4;

		anchor.Position = { (i - 2.0f) * 384.0f + 196.0f, 320.0f };

		animator.AnimationMap["default"] = defaultAnimations[i];
		animator.AnimationMap["hold"] = holdAnimations[i];
		animator.AnimationMap["press"] = hitAnimations[i];
		animator.DefaultAnimation = "default";
		animator.SetState("default");

		noteButtons[i] = entity;
	}

	const char* splashesName[8] =
	{
		"note impact 1 purple",
		"note impact 1  blue",
		"note impact 1 green",
		"note impact 1 red",
		"note impact 2 purple",
		"note impact 2 blue",
		"note impact 2 green",
		"note impact 2 red",
	};

	const char* coverNames[4] =
	{
		"holdCoverEndPurple00",
		"holdCoverEndBlue00",
		"holdCoverEndGreen00",
		"holdCoverEndRed00",
	};

	const char* coverStrumNames[4] =
	{
		"holdCoverPurple00",
		"holdCoverBlue00",
		"holdCoverGreen00",
		"holdCoverRed00",
	};

	const char* coverFileNames[4] =
	{
		"textures/holdCoverPurple.xml",
		"textures/holdCoverBlue.xml",
		"textures/holdCoverGreen.xml",
		"textures/holdCoverRed.xml",
	};

	const char* coverImageNames[4] =
	{
		"textures/holdCoverPurple.png",
		"textures/holdCoverBlue.png",
		"textures/holdCoverGreen.png",
		"textures/holdCoverRed.png",
	};

	// Precache cover animations
	for (int i = 0; i < 4; i++)
	{
		auto anim = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(24)
			.SetLoop(false)
			.SetNextState("destroy")
			.SetFrames(SparrowReader::readXML(coverFileNames[i], coverNames[i], true, true));

		mNoteCoverEndAnimations[i] = anim;

		scene->Resources.LoadTextureImage(coverImageNames[i]);
	}

	// Spawn cover hold entities
	for (int i = 0; i < 4; i++)
	{
		auto handle = scene->Resources.LoadTextureImage(coverImageNames[i]);

		auto coverEntity = scene->EntityManager.CreateEntity();
		auto& coverSprite = scene->SpriteRenderers.Create(coverEntity);
		auto& coverAnimator = scene->SpriteAnimators.Create(coverEntity);
		auto& coverTransform = scene->Transforms.Create(coverEntity);
		auto& anchor = scene->GuiAnchors.Create(coverEntity);

		anchor.AnchorPoint = Stratum::GuiAnchorPoint::TOP;
		anchor.Position = { (i - 2.0f) * 384.0f + 196.0f, 320.0f };

		coverSprite.Enabled = false;
		coverSprite.TextureHandle = handle;
		coverSprite.RenderLayer = NOTE_EFFECT_LAYER;
		coverSprite.CameraLayer = 4;
		
		auto anim = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(24)
			.SetFrames(SparrowReader::readXML(coverFileNames[i], coverStrumNames[i], true, true))
			.SetAnimateOnIdle(true)
			.SetLoop(true);

		coverAnimator.AnimationMap["default"] = anim;
		coverAnimator.SetState("default");

		mNoteCovers[i] = coverEntity;
	}

	// Precache note splashes
	for (int i = 0; i < 8; i++)
	{
		auto anim = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(24)
			.SetLoop(false)
			.SetNextState("destroy")
			.SetFrames(SparrowReader::readXML("textures/noteSplashes.xml", splashesName[i], true));

		mNoteSplashesAnimations[i] = anim;
	}

	mScoreTextEntity = scene->EntityManager.CreateEntity();
	mSubtitlesTextEntity = scene->EntityManager.CreateEntity();

	scene->TextComponents.Create(mScoreTextEntity);
	scene->TextComponents.Get(mScoreTextEntity).FontSize = 64.0f;
	scene->TextRenderers.Create(mScoreTextEntity).Alignment = 0.5f;
	scene->TextRenderers.Get(mScoreTextEntity).RenderLayer = 1000;
	scene->TextRenderers.Get(mScoreTextEntity).CameraLayer = 5;
	scene->Transforms.Create(mScoreTextEntity);
	scene->GuiAnchors.Create(mScoreTextEntity).AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM;
	scene->GuiAnchors.Get(mScoreTextEntity).Position.y += 60.0f;

	scene->TextComponents.Create(mSubtitlesTextEntity);
	scene->TextComponents.Get(mSubtitlesTextEntity).FontSize = 80.0f;
	scene->TextRenderers.Create(mSubtitlesTextEntity).Alignment = 0.5f;
	scene->TextRenderers.Get(mSubtitlesTextEntity).RenderLayer = 1001;
	scene->TextRenderers.Get(mSubtitlesTextEntity).CameraLayer = 4;
	scene->Transforms.Create(mSubtitlesTextEntity);
	scene->GuiAnchors.Create(mSubtitlesTextEntity).AnchorPoint = Stratum::GuiAnchorPoint::BOTTOM;
	scene->GuiAnchors.Get(mSubtitlesTextEntity).Position.y += 200.0f;

	this->RegisterEventHandler("Subtitles", [this, scene](ChartEvent& event)
		{
			auto& textComponent = scene->TextComponents.Get(mSubtitlesTextEntity);
			textComponent.Text = Stratum::Utils::ToWideString(event.Arg1);
		});
}

void Funkin::Conductor::PostUpdate(Stratum::Scene* scene)
{
}

void Funkin::Conductor::LoadChart(Stratum::Scene* scene, const std::string& path)
{
	chart = ChartLoader::LoadChart(path);
	SongTime = 0.0f;
	mSustainHeld = {};

	for (int j = 0; j < chart.sections.size(); j++)
	{
		auto& section = chart.sections[j];
		for (int i = 0; i < section.notes.size(); i++)
		{
			SpawnNote(scene, section.notes[i], i, j);
		}
	}

}

void Funkin::Conductor::Update(Stratum::Scene* scene)
{
	if (IsPaused)
	{
		return;
	}

	const int NOTE_MISS_SCORE = 100;

	BeatCountF = (chart.info.bpm / 60.0f) * SongTime;
	BeatCount = glm::floor(BeatCountF);

	const float stepsPerSecond = 1.0f / ((chart.info.bpm / 60.0f) * 4.0f);
	const uint32_t expectedStepCount = glm::floor(BeatCountF * 4.0f);

	auto lastStepCount = mStepCount;

	uint32_t simulatedSteps = expectedStepCount - lastStepCount;

	// Big lag spike! (TO DO: Fix the engine/get a good pc)
	// Simulate last 64 steps (Also helps during development when skipping parts of the song)
	if (simulatedSteps > 64)
	{
		mStepCount += simulatedSteps - 64;
		simulatedSteps = 64;
	}

	// Need to do this to avoid skipping steps in case of lag
	while (simulatedSteps > 0)
	{
		simulatedSteps -= 1;
		OnStep();
		mStepCount += 1;
	}

	bool botEnabled = EnableBot || false || BotPlay;

	auto notesManager = scene->GetComponentManager<NoteComponent>(C_NOTE_COMPONENT_NAME);
	auto noteHoldManager = scene->GetComponentManager<NoteHoldComponent>(C_NOTE_HOLD_COMPONENT_NAME);
	auto effectManager = scene->GetComponentManager<AnimatedEffectComponent>(C_ANIMATED_EFFECT_COMPONENT_NAME);

	auto& notes = notesManager->GetEntities();
	auto& effects = effectManager->GetEntities();

	std::array<bool, 4> inputs = {
		Stratum::Input::GetInputDown("funkin_left"),
		Stratum::Input::GetInputDown("funkin_down"),
		Stratum::Input::GetInputDown("funkin_up"),
		Stratum::Input::GetInputDown("funkin_right"),
	};

	std::array<bool, 4> inputsHold = {
		Stratum::Input::GetInput("funkin_left")  || botEnabled,
		Stratum::Input::GetInput("funkin_down")  || botEnabled,
		Stratum::Input::GetInput("funkin_up")	 || botEnabled,
		Stratum::Input::GetInput("funkin_right") || botEnabled,
	};

	std::array<nvrhi::static_vector<Stratum::ECS::edict_t, 16>, 4> hitNotes;

	const float SAFEZONE_PLUS = SongTime + SAFE_ZONE * 0.8f;
	const float SAFEZONE_MINUS = SongTime - SAFE_ZONE;

	for (auto entity : notes)
	{
		auto& note = notesManager->Get(entity);
		auto& transform = scene->Transforms.Get(entity);
		auto& buttonTransform = scene->Transforms.Get(noteButtons[note.NoteType]);

		if (note.IsOponent)
		{
			if (note.Time < SongTime && note.Time > SAFEZONE_MINUS) {
				Stratum::EventBus::InvokeEvent(NoteEvent{ true, false, false, note.NoteType, note.NoteIndex, note.SectionIndex });
				scene->EntityManager.DestroyEntity(entity);
			}
			continue;
		}

		STRUM_LINE_Y = buttonTransform.Position.y;

		float y = (STRUM_LINE_Y + 0.0f + (SongTime - note.Time) * (400.0f * chart.info.speed * 3.0f));

		if (inputs[note.NoteType])
		{
			if (note.Time < SAFEZONE_PLUS
				&& note.Time > SAFEZONE_MINUS) {
				if (hitNotes[note.NoteType].size() < hitNotes[note.NoteType].max_size() - 2)
					hitNotes[note.NoteType].push_back(entity);
			}
		}

		if (botEnabled)
		{
			if (note.Time < SongTime && note.Time > SAFEZONE_MINUS) {
				if (hitNotes[note.NoteType].size() < hitNotes[note.NoteType].max_size() - 2)
					hitNotes[note.NoteType].push_back(entity);
			}
		}

		glm::vec3 position = transform.Position;

		position.x = buttonTransform.Position.x;
		position.y = y;

		transform.SetPosition(position);

		if (y > (160.0f + 384.0f) + STRUM_LINE_Y)
		{
			scene->EntityManager.DestroyEntity(entity);

			MissCount++;

			PlayerScore -= NOTE_MISS_SCORE;

			Stratum::EventBus::InvokeEvent(NoteEvent{ false, true, false, note.NoteType, note.NoteIndex, note.SectionIndex });

			if (note.Sustain != Stratum::ECS::C_INVALID_ENTITY)
			{
				scene->EntityManager.DestroyEntity(note.Sustain);

				MissCount++;

				auto& sustain = noteHoldManager->Get(note.Sustain);

				scene->EntityManager.DestroyEntity(sustain.SustainEndSprite);

				PlayerScore -= NOTE_MISS_SCORE;
			}
		}

		if (note.Sustain != Stratum::ECS::C_INVALID_ENTITY)
		{
			auto& sustain = noteHoldManager->Get(note.Sustain);
			auto& sustainTransform = scene->Transforms.Get(note.Sustain);
			auto& sustainEndTransform = scene->Transforms.Get(sustain.SustainEndSprite);

			float y1 = sustain.HoldTime * 400.0f * chart.info.speed * 3.0f;
			float scaleY = y1 / 87.0f * 0.5f;
			
			sustainTransform.SetScale(glm::vec3(1.0f, scaleY, 1.0f));
			sustainTransform.SetPosition(position);

			position.y -= y1;
			sustainEndTransform.SetPosition(position);

		}

	}

	for (int i = 0; i < 4; i++)
	{
		auto& note = hitNotes[i];

		int sortedIndex = -1;
		float maxStrumTime = 999;

		for (int j = 0; j < note.size(); j++) {

			auto entity = note[j];
			auto& noteEntity = notesManager->Get(entity);

			if (noteEntity.Time < maxStrumTime) {
				maxStrumTime = noteEntity.Time;
				sortedIndex = j;
			}

		}

		if (sortedIndex != -1)
		{
			auto entity = note[sortedIndex];
			auto& noteEntity = notesManager->Get(entity);

			scene->EntityManager.DestroyEntity(entity);

			float diff = noteEntity.Time - SongTime;
			int32_t diffMillis = diff * 1000.0f;

			if (glm::abs(diffMillis) <= SICK_TIME_WINDOW / 2)
			{
				SpawnNoteSplash(scene, noteEntity.NoteType);
			}

			AddScoreNoteHit(diffMillis);

			if (noteEntity.Sustain != Stratum::ECS::C_INVALID_ENTITY)
			{
				if (mSustainHeld[noteEntity.NoteType] != 0)
				{
					scene->EntityManager.DestroyEntity(mSustainHeld[noteEntity.NoteType]);
				}
				mSustainHeld[noteEntity.NoteType] = noteEntity.Sustain;

				auto& sustainNote = noteHoldManager->Get(noteEntity.Sustain);

				sustainNote.HoldTime -= glm::max(diff, 0.0f);
			}

			Stratum::EventBus::InvokeEvent(NoteEvent{ false, false, false, noteEntity.NoteType, noteEntity.NoteIndex, noteEntity.SectionIndex });

			auto& animator = scene->SpriteAnimators.Get(noteButtons[noteEntity.NoteType]);
			animator.SetState("press");

		}


	}

	bool anySustainHeld = false;

	for (int i = 0; i < mSustainHeld.size(); i++)
	{
		auto ent = mSustainHeld[i];
		auto& coverSprite = scene->SpriteRenderers.Get(mNoteCovers[i]);

		if (ent != Stratum::ECS::C_INVALID_ENTITY)
		{
			anySustainHeld = true;
			auto& sustainNote = noteHoldManager->Get(ent);
			coverSprite.Enabled = true;

			if (!inputsHold[i])
			{
				scene->EntityManager.DestroyEntity(ent);
				scene->EntityManager.DestroyEntity(sustainNote.SustainEndSprite);
				mSustainHeld[i] = Stratum::ECS::C_INVALID_ENTITY;
				MissCount++;
				continue;
			}

			auto& sustainTransform = scene->Transforms.Get(ent);
			auto& sustainSprite = scene->SpriteRenderers.Get(ent);
			auto& sustainEndSprite = scene->SpriteRenderers.Get(sustainNote.SustainEndSprite);
			auto& sustainEndTransform = scene->Transforms.Get(sustainNote.SustainEndSprite);

			float holdTime = SongTime - sustainNote.Time;

			float colorStrenght = glm::abs(glm::sin(SongTime * glm::pi<float>() * GetConductorBeatMultiplier())) * 0.3f + 1.0f;

			sustainSprite.SpriteColor = glm::vec4(glm::vec3(colorStrenght), 1.0f);
			sustainEndSprite.SpriteColor = glm::vec4(glm::vec3(colorStrenght), 1.0f);

			Stratum::EventBus::InvokeEvent(NoteEvent{ false, false, true, sustainNote.NoteType, 0, 0 });

			float y = STRUM_LINE_Y;
			float y1 = (sustainNote.HoldTime - holdTime) * 400.0f * chart.info.speed * 3.0f;
			float scaleY = y1 / 87.0f / 2.0f;

			auto position = sustainTransform.Position;
			position.y = y;

			sustainTransform.SetScale(glm::vec3(1.0f, scaleY, 1.0f));
			sustainTransform.SetPosition(position);

			position.y -= y1;
			sustainEndTransform.SetPosition(position);

			if (sustainNote.HoldTime <= holdTime)
			{
				scene->EntityManager.DestroyEntity(sustainNote.SustainEndSprite);
				scene->EntityManager.DestroyEntity(ent);
				mSustainHeld[i] = Stratum::ECS::C_INVALID_ENTITY;
				SpawnSustainCover(scene, i);
				// PlayerScore += (sustainNote.HoldTime * 250.0f) * GetConductorBeatMultiplier();
			}

			auto& animator = scene->SpriteAnimators.Get(noteButtons[sustainNote.NoteType]);
			if (animator.AnimationMap[animator.CurrentAnimation].FrameIndex >= 5 || animator.CurrentAnimation.compare("press"))
			{
				animator.SetState("press");
			}
		}
		else
		{
			coverSprite.Enabled = false;
		}
	}

	if (anySustainHeld)
	{
		Stratum::Input::SetGamepadRumble(0.25f, 0.25f, 100);
	}

	for (auto entity : effects)
	{
		auto& animator = scene->SpriteAnimators.Get(entity);
		if (animator.CurrentAnimation.compare("destroy") == 0)
		{
			scene->EntityManager.DestroyEntity(entity);
		}
	}

	for (int i = 0; i < 4; i++)
	{
		auto& animator = scene->SpriteAnimators.Get(noteButtons[i]);
		auto& sprite = scene->SpriteRenderers.Get(noteButtons[i]);

		if (animator.CurrentAnimation.compare("press") == 0)
		{
			sprite.SpriteColor = glm::vec4(glm::vec3(1.15f), 1.0f);
		}
		else
		{
			sprite.SpriteColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		if (inputsHold[i] && !botEnabled)
		{
			if (animator.CurrentAnimation.compare("default") == 0)
			{
				animator.SetState("hold");
			}
		}
		else
		{
			if (animator.CurrentAnimation.compare("hold") == 0)
			{
				animator.SetState("default");
			}
		}
		
	}

	for (auto& event : chart.events)
	{
		if (!event.Triggered)
		{
			if (event.EventTime <= SongTime)
			{
				if (mEventHandlers.contains(event.EventName))
				{
					mEventHandlers[event.EventName](event);
				}
				else
				{
					Z_WARN("Event: [{}] has not handlers registered! ignoring {}, {}.", event.EventName, event.Arg1, event.Arg2);
				}
				event.Triggered = true;
			}
		}
	}

	Accuracy = 0.0f;

	for (int i = 0; i < mAccuracyList.size(); i++)
	{
		Accuracy += mAccuracyList[i] / mAccuracyList.size();
	}

	if (mAccuracyList.empty())
		Accuracy = 1.0f;

	auto& scoreText = scene->TextComponents.Get(mScoreTextEntity);

	scoreText.Text = Stratum::Utils::FormatString(L"Score: {} | Misses: {} | Accuracy: {:.2f}%\nStep: {}", this->PlayerScore, this->MissCount, this->Accuracy * 100.0f, this->mStepCount);
}

void Funkin::Conductor::RegisterEventHandler(const std::string& eventName, ChartEventHandler handler)
{
	mEventHandlers[eventName] = handler;
}

void Funkin::Conductor::AddScriptedEvent(int step, ScriptedEvent event)
{
	mScriptedEvents.push_back(ScriptedEventContainer{ event, false, step });
}

float Funkin::Conductor::StepsToSeconds(uint32_t stepCount)
{
	return static_cast<float>(stepCount) / 4.0f / GetConductorBeatMultiplier();
}

float Funkin::Conductor::GetConductorBeatMultiplier()
{
	return chart.info.bpm / 60.0f;
}

std::string Funkin::Conductor::GetPlayer1Name() const
{
	return chart.info.player1;
}

std::string Funkin::Conductor::GetPlayer2Name() const
{
	return chart.info.player2;
}

Funkin::ChartNote& Funkin::Conductor::GetNoteByIndex(uint32_t sectionIndex, uint32_t noteIndex)
{
	return chart.sections[sectionIndex].notes[noteIndex];
}

uint32_t Funkin::Conductor::GetStepCount()
{
	return mStepCount;
}

void Funkin::Conductor::OnStep()
{
	for (int i = 0; i < mSustainHeld.size(); i++)
	{
		auto ent = mSustainHeld[i];

		if (ent != Stratum::ECS::C_INVALID_ENTITY)
		{
			PlayerScore += 20;
		}
	}

	for (auto& event : mScriptedEvents)
	{
		if (event.stepCount == mStepCount)
		{
			event.event();
			event.executed = true;
		}
	}
}

void Funkin::Conductor::SpawnNote(Stratum::Scene* scene, ChartNote note, uint32_t index, uint32_t sectionIndex)
{
	int l = note.noteType;
	bool valid = false;
	bool isOponent = false;

	if (l <= 3 && note.mustHitSection) {
		valid = true;
	}
	else
	{
		l -= 4;
		if (l >= 0 && l <= 3 && !note.mustHitSection) {
			valid = true;
		}
	}

	if (!valid)
	{
		l = note.noteType;

		if (l <= 3 && !note.mustHitSection) {
			valid = true;
		}
		else
		{
			l -= 4;
			if (l >= 0 && l <= 3 && note.mustHitSection) {
				valid = true;
			}
		}

		isOponent = valid;
	}

	if (!valid)
	{
		return;
	}

	auto notesManager = scene->GetComponentManager<NoteComponent>(C_NOTE_COMPONENT_NAME);
	auto noteHoldManager = scene->GetComponentManager<NoteHoldComponent>(C_NOTE_HOLD_COMPONENT_NAME);

	auto entity = scene->EntityManager.CreateEntity();

	auto& enote = notesManager->Create(entity);
	auto& transform = scene->Transforms.Create(entity);
	auto& buttonTransform = scene->Transforms.Get(noteButtons[l]);

	enote.NoteIndex = index;
	enote.SectionIndex = sectionIndex;

	if (!isOponent)
	{
		auto& sprite = scene->SpriteRenderers.Create(entity);

		sprite.CameraLayer = 4;
		sprite.RenderLayer = NOTE_LAYER;

		if (l == 0)
			sprite.Rect = { glm::ivec2(630, 232), glm::ivec2(154, 157) };
		if (l == 1)
			sprite.Rect = { glm::ivec2(1850, 154), glm::ivec2(157, 154) };
		if (l == 2)
			sprite.Rect = { glm::ivec2(1850, 0), glm::ivec2(157, 154) };
		if (l == 3)
			sprite.Rect = { glm::ivec2(476, 232), glm::ivec2(154, 157) };

		sprite.TextureHandle = scene->Resources.LoadTextureImage("textures/NOTE_assets.DDS");
	}

	float y = (STRUM_LINE_Y + 0.0f + (SongTime - note.time) * (400.0f * chart.info.speed));

	enote.Time = note.time;
	enote.NoteType = l;
	enote.IsOponent = isOponent;

	transform.SetPosition(glm::vec3(buttonTransform.Position.x, y, 0.0f));

	if (note.holdTime > 0.01f && !isOponent)
	{
		auto edict = scene->EntityManager.CreateEntity();
		auto edictEnd = scene->EntityManager.CreateEntity();

		auto& noteHold = noteHoldManager->Create(edict);

		auto& noteSprite = scene->SpriteRenderers.Create(edict);
		auto& noteEndSprite = scene->SpriteRenderers.Create(edictEnd);

		auto& trans = scene->Transforms.Create(edict);
		auto& transEnd = scene->Transforms.Create(edictEnd);

		std::array<Stratum::SpriteRendererComponent::SpriteRect, 4> rects =
		{
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(1, 0), glm::ivec2(50, 87) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(105, 0), glm::ivec2(50, 87) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(209, 0), glm::ivec2(50, 87) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(313, 0), glm::ivec2(50, 87) }
		};

		std::array<Stratum::SpriteRendererComponent::SpriteRect, 4> endRects =
		{
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(53, 0), glm::ivec2(50, 64) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(157, 0), glm::ivec2(50, 64) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(261, 0), glm::ivec2(50, 64) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(365, 0), glm::ivec2(50, 64) },
		};

		noteSprite.Rect = rects[l];
		noteSprite.TextureHandle = scene->Resources.LoadTextureImage("textures/NOTE_hold_assets.png");
		noteSprite.RenderLayer = NOTE_HOLD_LAYER;
		noteSprite.Center = { 0.0f, -1.0f };
		noteSprite.CameraLayer = 4;

		noteEndSprite.Rect = endRects[l];
		noteEndSprite.Center = noteSprite.Center;
		noteEndSprite.TextureHandle = noteSprite.TextureHandle;
		noteEndSprite.RenderLayer = NOTE_HOLD_LAYER;
		noteEndSprite.CameraLayer = 4;

		noteHold.NoteType = l;
		noteHold.HoldTime = note.holdTime;
		noteHold.Time = note.time;
		noteHold.SustainEndSprite = edictEnd;

		enote.Sustain = edict;
	}

}

void Funkin::Conductor::SpawnNoteSplash(Stratum::Scene* scene, uint32_t noteType)
{
	auto effectManager = scene->GetComponentManager<AnimatedEffectComponent>(C_ANIMATED_EFFECT_COMPONENT_NAME);
	auto entity = scene->EntityManager.CreateEntity();
	auto& strumTransform = scene->Transforms.Get(noteButtons[noteType]);
	auto& transform = scene->Transforms.Create(entity);
	auto& sprite = scene->SpriteRenderers.Create(entity);
	auto& animator = scene->SpriteAnimators.Create(entity);
	effectManager->Create(entity);

	sprite.TextureHandle = scene->Resources.LoadTextureImage("textures/noteSplashes.png");
	sprite.RenderLayer = NOTE_EFFECT_LAYER;
	sprite.CameraLayer = 4;
	sprite.SpriteColor *= 1.15f;

	uint32_t offset = (rand() % 2) * 4;

	animator.AnimationMap["splash"] = mNoteSplashesAnimations[noteType + offset];
	animator.SetState("splash");

	transform.SetPosition(strumTransform.Position);
}

void Funkin::Conductor::SpawnSustainCover(Stratum::Scene* scene, uint32_t noteType)
{
	auto effectManager = scene->GetComponentManager<AnimatedEffectComponent>(C_ANIMATED_EFFECT_COMPONENT_NAME);
	auto entity = scene->EntityManager.CreateEntity();
	auto& strumTransform = scene->Transforms.Get(noteButtons[noteType]);
	auto& transform = scene->Transforms.Create(entity);
	auto& sprite = scene->SpriteRenderers.Create(entity);
	auto& animator = scene->SpriteAnimators.Create(entity);
	effectManager->Create(entity);

	const char* coverImageNames[4] =
	{
		"textures/holdCoverPurple.png",
		"textures/holdCoverBlue.png",
		"textures/holdCoverGreen.png",
		"textures/holdCoverRed.png",
	};

	sprite.TextureHandle = scene->Resources.LoadTextureImage(coverImageNames[noteType]);
	sprite.RenderLayer = NOTE_EFFECT_LAYER;
	sprite.CameraLayer = 4;
	sprite.SpriteColor *= 1.15f;

	animator.AnimationMap["coverEnd"] = mNoteCoverEndAnimations[noteType];
	animator.SetState("coverEnd");

	transform.SetPosition(strumTransform.Position);
}

void Funkin::Conductor::AddScoreNoteHit(int32_t time)
{

	const float SICK_TIME_FRAME = 45;
	const float GOOD_TIME_FRAME = 90;
	const float BAD_TIME_FRAME = 135;
	const float SHIT_TIME_FRAME = 160;

	const float SICK_MAX_SCORE = 500;
	const float SICK_MIN_SCORE = 354;

	const float GOOD_MAX_SCORE = 353;
	const float GOOD_MIN_SCORE = 38;

	const float BAD_MAX_SCORE = 38;
	const float BAD_MIN_SCORE = 10;

	const float SHIT_MAX_SCORE = 10;
	const float SHIT_MIN_SCORE = 9;

	float absTime = 1.0f - glm::max((float)glm::abs(time) - SICK_TIME_FRAME / 2.0f, 0.0f) / (SAFE_ZONE * 1000.0f);

	float score = -10.0f;

	if (glm::abs(time) <= SICK_TIME_FRAME)
	{
		float acc = time / SICK_TIME_FRAME;
		if (glm::abs(time) < SICK_TIME_FRAME / 2.0f)
		{
			acc = 1.0f;
			absTime = 1.0f;
		}
		score = glm::mix(SICK_MIN_SCORE, SICK_MAX_SCORE, acc);
	} 
	else if (glm::abs(time) <= GOOD_TIME_FRAME)
	{
		float acc = glm::abs(time) / GOOD_TIME_FRAME;
		score = glm::mix(GOOD_MIN_SCORE, GOOD_MAX_SCORE, acc);
	}
	else if (glm::abs(time) <= BAD_TIME_FRAME)
	{
		float acc = glm::abs(time) / BAD_TIME_FRAME;
		score = glm::mix(BAD_MIN_SCORE, BAD_MAX_SCORE, acc);
	}
	else if (glm::abs(time) <= SHIT_TIME_FRAME)
	{
		float acc = glm::abs(time) / SHIT_TIME_FRAME;
		score = glm::mix(SHIT_MIN_SCORE, SHIT_MAX_SCORE, acc);
	}

	score = glm::ceil(score);

	mAccuracyList.push_back(absTime);
	PlayerScore += (int32_t)score;
}