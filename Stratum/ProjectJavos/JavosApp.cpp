#include "JavosApp.h"

#include "InGameSystem.h"

#undef min
#undef max

Funkin::JavosApp::JavosApp(Stratum::ApplicationInfo appInfo) : Stratum::Application(appInfo)
{
}

void Funkin::JavosApp::OnInit()
{
	LoadChartParams params;
	params.ChartPath = "fnf/data/bite/bite-fernan.json";

	auto scene = new Stratum::Scene();
	SetScene(scene);

	scene->RegisterCustomSystem(new InGameSystem(params));
}

void Funkin::JavosApp::OnFrameUpdate()
{

}

void Funkin::JavosApp::OnFrameRenderImGui()
{
	
}

void Funkin::JavosApp::Cleanup()
{
	
}
