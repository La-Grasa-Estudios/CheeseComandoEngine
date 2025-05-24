#include <Input/Input.h>

#include "Conductor.h"
#include "Components.h"
#include "ChartLoader.h"
#include "SparrowReader.h"

#include <Core/Logger.h>
#include <Scene/Scene.h>
#include <Event/EventHandler.h>
#include <Util/Globals.h>

const float PRECISION = 64.0F;
const float DISPLACEMENT = -256.0F;
float STRUM_LINE_Y = 200;
const float SAFE_ZONE = 1.0f / 5.0f;

static const int SICK_TIME_WINDOW = 45;
static const int GOOD_TIME_WINDOW = 90;
static const int BAD_TIME_WINDOW = 130;

std::array<Stratum::ECS::edict_t, 4> noteButtons;

const int32_t NOTE_BUTTON_LAYER = 0;
const int32_t NOTE_HOLD_LAYER = 5;
const int32_t NOTE_LAYER = 10;
const int32_t NOTE_EFFECT_LAYER = 15;

Javos::Conductor::Conductor()
{
	mHitNoteEvent = Stratum::EventHandler::GetEventID("hit_note");
	mSustainNoteEvent = Stratum::EventHandler::GetEventID("sustain_note");
	mMissNoteEvent = Stratum::EventHandler::GetEventID("miss_note");

	RegisterEventHandler("StSetBot", [this](ChartEvent& event)
		{
			EnableBot = event.castBoolean(event.Arg1);
		});
}

void Javos::Conductor::Init(Stratum::Scene* scene)
{
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
		Stratum::SpriteAnimator::Animation().SetLoop(true).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "arrowLEFT00", true)),
		Stratum::SpriteAnimator::Animation().SetLoop(true).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "arrowDOWN00", true)),
		Stratum::SpriteAnimator::Animation().SetLoop(true).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "arrowUP00", true)),
		Stratum::SpriteAnimator::Animation().SetLoop(true).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "arrowRIGHT00", true)),
	};

	std::array<Stratum::SpriteAnimator::Animation, 4> holdAnimations =
	{
		Stratum::SpriteAnimator::Animation().SetTransitionToDefault(false).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "left press00", true)),
		Stratum::SpriteAnimator::Animation().SetTransitionToDefault(false).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "down press00", true)),
		Stratum::SpriteAnimator::Animation().SetTransitionToDefault(false).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "up press00", true)),
		Stratum::SpriteAnimator::Animation().SetTransitionToDefault(false).SetFrameRate(24).SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "right press00", true)),
	};

	std::array<Stratum::SpriteAnimator::Animation, 4> hitAnimations =
	{
		Stratum::SpriteAnimator::Animation().SetFrameRate(24).SetNextState("default").SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "left confirm00", true)),
		Stratum::SpriteAnimator::Animation().SetFrameRate(24).SetNextState("default").SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "down confirm00", true)),
		Stratum::SpriteAnimator::Animation().SetFrameRate(24).SetNextState("default").SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "up confirm00", true)),
		Stratum::SpriteAnimator::Animation().SetFrameRate(24).SetNextState("default").SetFrames(SparrowReader::readXML("textures/NOTE_assets.xml", "right confirm00", true)),
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
		sprite.IsGui = true;

		anchor.Position = { i * 384.0f + (250.0f), 320.0f };

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
			.SetFrames(SparrowReader::readXML(coverFileNames[i], coverNames[i], true));

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
		anchor.Position = { i * 384.0f + (250.0f), 320.0f };

		coverSprite.Enabled = false;
		coverSprite.TextureHandle = handle;
		coverSprite.RenderLayer = NOTE_EFFECT_LAYER;
		coverSprite.IsGui = true;
		
		auto anim = Stratum::SpriteAnimator::Animation()
			.SetFrameRate(24)
			.SetFrames(SparrowReader::readXML(coverFileNames[i], coverStrumNames[i], true))
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
}

void Javos::Conductor::PostUpdate(Stratum::Scene* scene)
{
}

void Javos::Conductor::LoadChart(Stratum::Scene* scene, const std::string& path)
{
	chart = ChartLoader::LoadChart(path);
	SongTime = 0.0f;
	mSustainHeld = {};

	for (auto section : chart.sections)
	{
		for (auto note : section.notes)
		{
			SpawnNote(scene, note);
		}
	}

}

void Javos::Conductor::Update(Stratum::Scene* scene)
{
	const int NOTE_MISS_SCORE = 100;

	BeatCountF = (chart.info.bpm / 60.0f) * SongTime;
	BeatCount = glm::floor(BeatCountF);

	bool botEnabled = EnableBot || false;

	auto notesManager = scene->GetComponentManager<NoteComponent>(C_NOTE_COMPONENT_NAME);
	auto noteHoldManager = scene->GetComponentManager<NoteHoldComponent>(C_NOTE_HOLD_COMPONENT_NAME);
	auto effectManager = scene->GetComponentManager<AnimatedEffectComponent>(C_ANIMATED_EFFECT_COMPONENT_NAME);

	auto& notes = notesManager->GetEntities();
	auto& effects = effectManager->GetEntities();

	nvrhi::static_vector<Stratum::ECS::edict_t, 64> entitiesToDestroy;

	std::array<bool, 4> inputs = { 
		Stratum::Input::GetKeyDown(KeyCode::A) || Stratum::Input::GetKeyDown(KeyCode::LEFT),
		Stratum::Input::GetKeyDown(KeyCode::S) || Stratum::Input::GetKeyDown(KeyCode::DOWN),
		Stratum::Input::GetKeyDown(KeyCode::W) || Stratum::Input::GetKeyDown(KeyCode::UP) ,
		Stratum::Input::GetKeyDown(KeyCode::D) || Stratum::Input::GetKeyDown(KeyCode::RIGHT)
	};

	std::array<bool, 4> inputsHold = {
		Stratum::Input::GetKey(KeyCode::A) || Stratum::Input::GetKey(KeyCode::LEFT) || botEnabled,
		Stratum::Input::GetKey(KeyCode::S) || Stratum::Input::GetKey(KeyCode::DOWN) || botEnabled,
		Stratum::Input::GetKey(KeyCode::W) || Stratum::Input::GetKey(KeyCode::UP) || botEnabled,
		Stratum::Input::GetKey(KeyCode::D) || Stratum::Input::GetKey(KeyCode::RIGHT) || botEnabled
	};

	std::array<nvrhi::static_vector<Stratum::ECS::edict_t, 16>, 4> hitNotes;

	const float SAFEZONE_PLUS = SongTime + SAFE_ZONE * 0.8f;
	const float SAFEZONE_MINUS = SongTime - SAFE_ZONE;

	const uint32_t maxNotesDestroyed = 8;
	uint32_t numNotesDestroyed = 0;

	for (auto entity : notes)
	{
		auto& note = notesManager->Get(entity);
		auto& transform = scene->Transforms.Get(entity);
		auto& buttonTransform = scene->Transforms.Get(noteButtons[note.NoteType]);

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
			numNotesDestroyed++;
			if (numNotesDestroyed < maxNotesDestroyed)
			{
				entitiesToDestroy.push_back(entity);

				PlayerScore -= NOTE_MISS_SCORE;

				Stratum::EventHandler::InvokeEvent(mMissNoteEvent, this);

				if (note.Sustain != Stratum::ECS::C_INVALID_ENTITY)
				{
					entitiesToDestroy.push_back(note.Sustain);

					auto& sustain = noteHoldManager->Get(note.Sustain);

					entitiesToDestroy.push_back(sustain.SustainEndSprite);

					PlayerScore -= NOTE_MISS_SCORE;
				}
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

			entitiesToDestroy.push_back(entity);

			float diff = SongTime - noteEntity.Time;
			int32_t diffMillis = diff * 1000.0f;

			if (diffMillis >= 0 && diffMillis <= SICK_TIME_WINDOW / 2)
			{
				SpawnNoteSplash(scene, noteEntity.NoteType);
			}

			AddScoreNoteHit(diffMillis);

			if (noteEntity.Sustain != Stratum::ECS::C_INVALID_ENTITY)
			{
				if (mSustainHeld[noteEntity.NoteType] != 0)
				{
					entitiesToDestroy.push_back(mSustainHeld[noteEntity.NoteType]);
				}
				mSustainHeld[noteEntity.NoteType] = noteEntity.Sustain;

				auto& sustainNote = noteHoldManager->Get(noteEntity.Sustain);

				sustainNote.HoldTime -= glm::max(diff, 0.0f);
			}

			Stratum::EventHandler::InvokeEvent(mHitNoteEvent, this, { (void*)noteEntity.NoteType }, 1);

			auto& animator = scene->SpriteAnimators.Get(noteButtons[noteEntity.NoteType]);
			animator.SetState("press");

		}


	}

	for (int i = 0; i < mSustainHeld.size(); i++)
	{
		auto ent = mSustainHeld[i];
		auto& coverSprite = scene->SpriteRenderers.Get(mNoteCovers[i]);

		if (ent != Stratum::ECS::C_INVALID_ENTITY)
		{
			auto& sustainNote = noteHoldManager->Get(ent);
			coverSprite.Enabled = true;

			if (!inputsHold[i])
			{
				entitiesToDestroy.push_back(ent);
				entitiesToDestroy.push_back(sustainNote.SustainEndSprite);
				mSustainHeld[i] = Stratum::ECS::C_INVALID_ENTITY;
				continue;
			}

			auto& sustainTransform = scene->Transforms.Get(ent);
			auto& sustainEndTransform = scene->Transforms.Get(sustainNote.SustainEndSprite);

			float holdTime = SongTime - sustainNote.Time;

			Stratum::EventHandler::InvokeEvent(mSustainNoteEvent, this, { (void*)sustainNote.NoteType }, 1);

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
				entitiesToDestroy.push_back(sustainNote.SustainEndSprite);
				entitiesToDestroy.push_back(ent);
				mSustainHeld[i] = Stratum::ECS::C_INVALID_ENTITY;
				SpawnSustainCover(scene, i);
			}

			auto& animator = scene->SpriteAnimators.Get(noteButtons[sustainNote.NoteType]);
			animator.SetState("press");
		}
		else
		{
			coverSprite.Enabled = false;
		}
	}

	for (auto entity : effects)
	{
		auto& animator = scene->SpriteAnimators.Get(entity);
		if (animator.CurrentAnimation.compare("destroy") == 0)
		{
			entitiesToDestroy.push_back(entity);
		}
	}

	for (auto entity : entitiesToDestroy)
	{
		scene->EntityManager.DestroyEntity(entity);
	}

	for (int i = 0; i < 4; i++)
	{
		auto& animator = scene->SpriteAnimators.Get(noteButtons[i]);
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
}

void Javos::Conductor::RegisterEventHandler(const std::string& eventName, ChartEventHandler handler)
{
	mEventHandlers[eventName] = handler;
}

void Javos::Conductor::SpawnNote(Stratum::Scene* scene, ChartNote note)
{
	int l = note.noteType;
	bool valid = false;

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
		return;

	auto notesManager = scene->GetComponentManager<NoteComponent>(C_NOTE_COMPONENT_NAME);
	auto noteHoldManager = scene->GetComponentManager<NoteHoldComponent>(C_NOTE_HOLD_COMPONENT_NAME);

	auto entity = scene->EntityManager.CreateEntity();

	auto& enote = notesManager->Create(entity);
	auto& transform = scene->Transforms.Create(entity);
	auto& buttonTransform = scene->Transforms.Get(noteButtons[l]);

	auto& sprite = scene->SpriteRenderers.Create(entity);

	sprite.IsGui = true;
	sprite.RenderLayer = NOTE_LAYER;
	
	if (l == 0)
	{
		sprite.Rect = { glm::ivec2(630, 232), glm::ivec2(154, 157) };
	}
	if (l == 1)
	{
		sprite.Rect = { glm::ivec2(1850, 154), glm::ivec2(157, 154) };
	}
	if (l == 2)
	{
		sprite.Rect = { glm::ivec2(1850, 0), glm::ivec2(157, 154) };
	}
	if (l == 3)
	{
		sprite.Rect = { glm::ivec2(476, 232), glm::ivec2(154, 157) };
	}

	sprite.TextureHandle = scene->Resources.LoadTextureImage("textures/NOTE_assets.DDS");

	float y = (STRUM_LINE_Y + 0.0f + (SongTime - note.time) * (400.0f * chart.info.speed));

	enote.Time = note.time;
	enote.NoteType = l;

	transform.SetPosition(glm::vec3(buttonTransform.Position.x, y, 0.0f));

	if (note.holdTime > 0.01f)
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
		noteSprite.IsGui = true;

		noteEndSprite.Rect = endRects[l];
		noteEndSprite.Center = noteSprite.Center;
		noteEndSprite.TextureHandle = noteSprite.TextureHandle;
		noteEndSprite.RenderLayer = NOTE_HOLD_LAYER;
		noteEndSprite.IsGui = true;

		noteHold.NoteType = l;
		noteHold.HoldTime = note.holdTime;
		noteHold.Time = note.time;
		noteHold.SustainEndSprite = edictEnd;

		enote.Sustain = edict;
	}

}

void Javos::Conductor::SpawnNoteSplash(Stratum::Scene* scene, uint32_t noteType)
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
	sprite.IsGui = true;

	uint32_t offset = (rand() % 2) * 4;

	animator.AnimationMap["splash"] = mNoteSplashesAnimations[noteType + offset];
	animator.SetState("splash");

	transform.SetPosition(strumTransform.Position);
}

void Javos::Conductor::SpawnSustainCover(Stratum::Scene* scene, uint32_t noteType)
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
	sprite.IsGui = true;

	animator.AnimationMap["coverEnd"] = mNoteCoverEndAnimations[noteType];
	animator.SetState("coverEnd");

	transform.SetPosition(strumTransform.Position);
}

void Javos::Conductor::AddScoreNoteHit(uint32_t time)
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

	float score = -10.0f;

	if (time >= 0 && time <= SICK_TIME_FRAME)
	{
		float acc = time / SICK_TIME_FRAME;
		if (time < SICK_TIME_FRAME / 2.0f)
		{
			acc = 1.0f;
		}
		score = glm::mix(SICK_MIN_SCORE, SICK_MAX_SCORE, acc);
	} 
	else if (time >= -GOOD_TIME_FRAME && time <= GOOD_TIME_FRAME)
	{
		float acc = glm::abs(time) / GOOD_TIME_FRAME;
		score = glm::mix(GOOD_MIN_SCORE, GOOD_MAX_SCORE, acc);
	}
	else if (time >= -BAD_TIME_FRAME && time <= BAD_TIME_FRAME)
	{
		float acc = glm::abs(time) / BAD_TIME_FRAME;
		score = glm::mix(BAD_MIN_SCORE, BAD_MAX_SCORE, acc);
	}
	else if (time >= -SHIT_TIME_FRAME && time <= SHIT_TIME_FRAME)
	{
		float acc = glm::abs(time) / SHIT_TIME_FRAME;
		score = glm::mix(SHIT_MIN_SCORE, SHIT_MAX_SCORE, acc);
	}

	score = glm::ceil(score);

	PlayerScore += (int32_t)score;
}
