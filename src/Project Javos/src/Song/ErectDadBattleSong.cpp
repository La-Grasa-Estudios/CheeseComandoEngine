#include "ErectDadBattleSong.h"

#include "SongCommon.h"

extern Funkin::GameState gGameState;

void Funkin::ErectDadBattleSong::Init(Conductor* pConductor, InGameSystem* pIngameSystem, Stratum::Scene* pScene)
{
	gGameState.DoBeatEveryNthBeat = 2;
	pConductor->RegisterEventHandler("Add Camera Zoom", [pIngameSystem](ChartEvent& e)
		{
			float f = e.castFloat(e.Arg1);
			float f2 = e.castFloat(e.Arg2);

			if (e.Arg1.empty())
				f = 0.015f;
			if (e.Arg2.empty())
				f2 = 0.03f;

			pIngameSystem->CameraZoomModifier += f;
			pIngameSystem->GuiZoomModifier += f;
		});
	pConductor->RegisterEventHandler("Set CamZoom", [pIngameSystem](ChartEvent& e)
		{
			float f = e.castFloat(e.Arg1);
			if (e.Arg1.empty())
				f = 1.0f;
			pIngameSystem->CameraZoomModifier = f;
		});
	pConductor->RegisterEventHandler("Play Animation", [pIngameSystem](ChartEvent& e)
		{
			if (e.Arg2.compare("Dad") == 0)
			{
				CharaRegistry::GetCharacter("erect-dad")->PlayAnimation(e.Arg1);
			}
		});
	pConductor->RegisterEventHandler("Camera Flash", [pConductor](ChartEvent& e)
		{
			ChartEvent EventWhite{};
			ChartEvent EventStopWhite{};
			EventWhite.EventName = "StFadeToWhite";
			EventStopWhite.EventName = "StFadeToWhite";
			EventWhite.EventTime = pConductor->SongTime + 0.05f;
			EventStopWhite.EventTime = pConductor->SongTime + 0.15f;
			EventWhite.Arg1 = "1.0";
			EventWhite.Arg2 = "1";
			EventStopWhite.Arg1 = "0.0";
			EventStopWhite.Arg2 = std::to_string((int)(e.castFloat(e.Arg1) * 1000.0f));
			pConductor->chart.events.push_back(EventWhite);
			pConductor->chart.events.push_back(EventStopWhite);
		});

	pConductor->AddScriptedEvent(385, [&] {
		//CharaRegistry::GetCharacter("erect-bf")->DoDanceBeatOverride = 0;
		});

	auto gf = CharaRegistry::GetCharacter("erect-bf");

	if (gf)
	{
		CharaRegistry::GetCharacter("erect-dad")->AddAnimation("scared", "DAD SCARED", "", 30, true, false);
		gf->AddAnimation("hey", "BF HEY!!", "idle", 14, false, false);
		gf->DoDanceBeatOverride = 4;
		gf->SetEnabled(true);
		gf->SetLayer(9);
	}

	CharaRegistry::AddCharacter("erect-gf");
	CharaRegistry::GetCharacter("erect-gf")->SetEnabled(true);
	CharaRegistry::GetCharacter("erect-gf")->SetLayer(9);
	
}

void Funkin::ErectDadBattleSong::Update()
{
}

void Funkin::ErectDadBattleSong::OnStep(int step)
{
}
