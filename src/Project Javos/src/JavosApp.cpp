#include "JavosApp.h"

#include "MainMenuSystem.h"
#include "Cursed/BalatroSystem.h"

#include <DevTools/ShaderCompiler.h>

#undef min
#undef max

Funkin::JavosApp::JavosApp(Stratum::ApplicationInfo appInfo) : Stratum::Application(appInfo)
{
}

void Funkin::JavosApp::OnInit()
{
	srand(static_cast<unsigned int>(time(nullptr)));

	using namespace Stratum;
	ShaderCompiler::build_object("shaders/2d/balatro_bg.hlsl", "Data/shaders/2d/balatro_bg.cso", ShaderCompiler::shader_type::vertex);
	ShaderCompiler::build_object("shaders/2d/balatro_dissolve.hlsl", "Data/shaders/2d/balatro_dissolve.cso", ShaderCompiler::shader_type::vertex);

	auto scene = new Stratum::Scene();
	SetScene(scene);

	//scene->RegisterCustomSystem(new BalatroSystem());
	//scene->RegisterCustomSystem(new LoadingScreenSystem(params));
	scene->RegisterCustomSystem(new MainMenuSystem());
}

void Funkin::JavosApp::OnFrameUpdate()
{

}

void Funkin::JavosApp::OnFrameRenderImGui()
{
	Application::OnFrameRenderImGui();
}

void Funkin::JavosApp::Cleanup()
{
	
}
