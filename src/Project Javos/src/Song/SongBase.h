#pragma once

namespace Stratum
{
	class Scene;
}

namespace Funkin
{
	class Conductor;
	class InGameSystem;

	class SongBase
	{
	public:
		virtual void Init(Conductor* pConductor, InGameSystem* pIngameSystem, Stratum::Scene* pScene) = 0;
		virtual void Update() {};
		virtual void OnSongStart() {};
		virtual void OnPause() {};
		virtual void OnUnpause() {};
		virtual void OnStep(int step) {};
	protected:
		Stratum::Scene* mScene;
	};

}