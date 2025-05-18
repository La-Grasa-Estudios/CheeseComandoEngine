#include <Input/Input.h>

#include "Conductor.h"
#include "Components.h"
#include "ChartLoader.h"
#include "SparrowReader.h"

#include <Scene/Scene.h>
#include <Event/EventHandler.h>
#include <Util/Globals.h>

const float PRECISION = 64.0F;
const float DISPLACEMENT = -256.0F;
const float STRUM_LINE_Y = 200;
const float SAFE_ZONE = 1.0f / 5.0f;

std::array<Stratum::ECS::edict_t, 4> noteButtons;

Javos::Conductor::Conductor()
{
	mHitNoteEvent = Stratum::EventHandler::GetEventID("hit_note");
}

void Javos::Conductor::Init(Stratum::Scene* scene)
{
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

		sprite.RenderLayer = Stratum::SpriteRendererComponent::LAYER_FG;
		sprite.Rect = rects[i];
		sprite.TextureHandle = scene->Resources.LoadTextureImage("textures/NOTE_assets.DDS");

		transform.SetPosition({ i * 96.0f, STRUM_LINE_Y, 0.0f });
		transform.SetScale(glm::vec3(0.25f));

		animator.AnimationMap["default"] = defaultAnimations[i];
		animator.AnimationMap["hold"] = holdAnimations[i];
		animator.AnimationMap["press"] = hitAnimations[i];
		animator.DefaultAnimation = "default";
		animator.SetState("default");

		noteButtons[i] = entity;
	}

}

void Javos::Conductor::Update(Stratum::Scene* scene)
{

	BeatCountF = (chart.info.bpm / 60.0f) * SongTime;
	BeatCount = glm::floor(BeatCountF);

	auto notesManager = scene->GetComponentManager<NoteComponent>(C_NOTE_COMPONENT_NAME);
	auto noteHoldManager = scene->GetComponentManager<NoteHoldComponent>(C_NOTE_HOLD_COMPONENT_NAME);

	auto& notes = notesManager->GetEntities();

	nvrhi::static_vector<Stratum::ECS::edict_t, 32> notesToDestroy;

	std::array<bool, 4> inputs = { 
		Stratum::Input::GetKeyDown(KeyCode::A) || Stratum::Input::GetKeyDown(KeyCode::LEFT),
		Stratum::Input::GetKeyDown(KeyCode::S) || Stratum::Input::GetKeyDown(KeyCode::DOWN),
		Stratum::Input::GetKeyDown(KeyCode::W) || Stratum::Input::GetKeyDown(KeyCode::UP) ,
		Stratum::Input::GetKeyDown(KeyCode::D) || Stratum::Input::GetKeyDown(KeyCode::RIGHT)
	};

	std::array<bool, 4> inputsHold = {
		Stratum::Input::GetKey(KeyCode::A) || Stratum::Input::GetKey(KeyCode::LEFT),
		Stratum::Input::GetKey(KeyCode::S) || Stratum::Input::GetKey(KeyCode::DOWN),
		Stratum::Input::GetKey(KeyCode::W) || Stratum::Input::GetKey(KeyCode::UP) ,
		Stratum::Input::GetKey(KeyCode::D) || Stratum::Input::GetKey(KeyCode::RIGHT)
	};

	std::array<nvrhi::static_vector<Stratum::ECS::edict_t, 16>, 4> hitNotes;

	const float SAFEZONE_PLUS = SongTime + SAFE_ZONE * 0.8f;
	const float SAFEZONE_MINUS = SongTime - SAFE_ZONE;

	for (auto entity : notes)
	{
		auto& note = notesManager->Get(entity);
		auto& transform = scene->Transforms.Get(entity);

		float y = (STRUM_LINE_Y + 0.0f + (SongTime - note.Time) * (100.0f * chart.info.speed * 3.0f));

		if (inputs[note.NoteType])
		{
			if (note.Time < SAFEZONE_PLUS
				&& note.Time > SAFEZONE_MINUS) {
				hitNotes[note.NoteType].push_back(entity);
			}
		}

		glm::vec3 position = transform.Position;
		position.y = y;
		transform.SetPosition(position);

		if (note.Sustain != Stratum::ECS::C_INVALID_ENTITY)
		{
			auto& sustain = noteHoldManager->Get(note.Sustain);
			auto& sustainTransform = scene->Transforms.Get(note.Sustain);

			float y1 = sustain.HoldTime * 100.0f * chart.info.speed * 3.0f;
			float scaleY = y1 / 43.5f;
			
			sustainTransform.SetScale(glm::vec3(0.25f, scaleY * 0.25f, 1.0f));
			sustainTransform.SetPosition(position);
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

			notesToDestroy.push_back(entity);

			if (noteEntity.Sustain != Stratum::ECS::C_INVALID_ENTITY)
			{
				if (mSustainHeld[noteEntity.NoteType] != 0)
				{
					notesToDestroy.push_back(mSustainHeld[noteEntity.NoteType]);
				}
				mSustainHeld[noteEntity.NoteType] = noteEntity.Sustain;

				auto& sustainNote = noteHoldManager->Get(noteEntity.Sustain);

				float diff = SongTime - noteEntity.Time;

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
		if (ent != Stratum::ECS::C_INVALID_ENTITY)
		{
			if (!inputsHold[i])
			{
				notesToDestroy.push_back(ent);
				mSustainHeld[i] = Stratum::ECS::C_INVALID_ENTITY;
				continue;
			}

			auto& sustainNote = noteHoldManager->Get(ent);
			auto& sustainTransform = scene->Transforms.Get(ent);

			float holdTime = SongTime - sustainNote.Time;

			Stratum::EventHandler::InvokeEvent(mHitNoteEvent, this, { (void*)sustainNote.NoteType }, 1);

			float y = STRUM_LINE_Y;
			float y1 = (sustainNote.HoldTime - holdTime) * 100.0f * chart.info.speed * 3.0f;
			float scaleY = y1 / 43.5f;

			auto position = sustainTransform.Position;
			position.y = y;

			sustainTransform.SetScale(glm::vec3(0.25f, scaleY * 0.25f, 1.0f));
			sustainTransform.SetPosition(position);

			if (sustainNote.HoldTime <= holdTime)
			{
				notesToDestroy.push_back(ent);
				mSustainHeld[i] = Stratum::ECS::C_INVALID_ENTITY;
			}

			auto& animator = scene->SpriteAnimators.Get(noteButtons[sustainNote.NoteType]);
			animator.SetState("press");
		}
	}

	for (auto entity : notesToDestroy)
	{
		scene->EntityManager.DestroyEntity(entity);
	}

	for (int i = 0; i < 4; i++)
	{
		auto& animator = scene->SpriteAnimators.Get(noteButtons[i]);
		if (inputsHold[i])
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
	auto& sprite = scene->SpriteRenderers.Create(entity);

	sprite.RenderLayer = Stratum::SpriteRendererComponent::LAYER_FG1;
	
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

	float y = (STRUM_LINE_Y + 0.0f + (SongTime - note.time) * (100.0f * chart.info.speed));

	enote.Time = note.time;
	enote.NoteType = l;

	transform.SetPosition(glm::vec3(l * 96.0f, y, 0.0f));
	transform.SetScale(glm::vec3(0.25f));

	if (note.holdTime > 0.01f)
	{
		auto edict = scene->EntityManager.CreateEntity();
		auto& noteHold = noteHoldManager->Create(edict);
		auto& noteSprite = scene->SpriteRenderers.Create(edict);
		auto& trans = scene->Transforms.Create(edict);

		std::array<Stratum::SpriteRendererComponent::SpriteRect, 4> rects =
		{
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(1, 0), glm::ivec2(50, 87) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(105, 0), glm::ivec2(50, 87) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(209, 0), glm::ivec2(50, 87) },
			Stratum::SpriteRendererComponent::SpriteRect{ glm::ivec2(313, 0), glm::ivec2(50, 87) }
		};

		noteSprite.Rect = rects[l];
		noteSprite.TextureHandle = scene->Resources.LoadTextureImage("textures/NOTE_hold_assets.png");
		noteSprite.RenderLayer = Stratum::SpriteRendererComponent::LAYER_FG0;
		noteSprite.Center = { 0.0f, -1.0f };

		noteHold.NoteType = l;
		noteHold.HoldTime = note.holdTime;
		noteHold.Time = note.time;

		enote.Sustain = edict;
	}

}

bool Javos::Conductor::CheckNoteInput(float time)
{
	return false;
}
