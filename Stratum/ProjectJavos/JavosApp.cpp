#include "JavosApp.h"

#include "InGameSystem.h"

#undef min
#undef max

Javos::JavosApp::JavosApp(Stratum::ApplicationInfo appInfo) : Stratum::Application(appInfo)
{
}

void Javos::JavosApp::OnInit()
{
	LoadChartParams params;
	params.ChartPath = "fnf/data/bite/bite-fernan.json";

	auto scene = new Stratum::Scene();
	SetScene(scene);

	scene->RegisterCustomSystem(new InGameSystem(params));
}

void Javos::JavosApp::OnFrameUpdate()
{

}

void Javos::JavosApp::OnFrameRenderImGui()
{
	
}

void Javos::JavosApp::Cleanup()
{
	
}
