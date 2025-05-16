#pragma once

#include "Core/Application.h"

namespace Javos
{
	class JavosApp : public Stratum::Application
	{
	public:
		JavosApp(Stratum::ApplicationInfo appInfo);

		void OnInit();
		void OnFrameUpdate();

		void OnFrameRenderImGui();
		void Cleanup();
	};
}