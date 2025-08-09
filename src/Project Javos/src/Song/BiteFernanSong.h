#pragma once

#include "SongBase.h"

namespace Funkin
{
	class BiteFernanSong : public SongBase
	{
	public:
		void Init(Conductor* pConductor, InGameSystem* pIngameSystem, Stratum::Scene* pScene) override;
		void Update() override;
		void OnStep(int step) override;
	private:
		Conductor* mConductor;
		InGameSystem* mIngameSystem;
		TimedActionSystem* pTimedActionSystem;
	};
}