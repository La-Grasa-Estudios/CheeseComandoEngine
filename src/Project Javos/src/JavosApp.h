#pragma once

#include "Core/Application.h"

namespace Funkin
{
	class JavosApp : public Stratum::Application
	{
	public:
		JavosApp(Stratum::ApplicationInfo appInfo);

		void OnEarlyInit() override;
		void OnInit();
		void OnFrameUpdate();

		void OnFrameRenderImGui();
		void Cleanup();
	};
}