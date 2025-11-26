#include "JavosApp.h"

#include "Settings.h"
#include "MainMenuSystem.h"
#include "Cursed/BalatroSystem.h"
#include <Event/EventBus.h>
#include <AngelScript/AngelScript.h>
#include <angelscript.h>

#include <DevTools/ShaderCompiler.h>

#undef min
#undef max

static bool GetValue(std::string& key, bool defaultValue)
{
	return Funkin::Settings::s_Settings->Get(key.c_str(), defaultValue);
}

static int GetValue(std::string& key, int defaultValue)
{
	return Funkin::Settings::s_Settings->Get(key.c_str(), defaultValue);
}

static float GetValue(std::string& key, float defaultValue)
{
	return Funkin::Settings::s_Settings->Get(key.c_str(), defaultValue);
}

static void SetValue(std::string& key, bool value)
{
	Funkin::Settings::s_Settings->Get(key.c_str(), value).boolValue = value;
}

static void SetValue(std::string& key, int value)
{
	Funkin::Settings::s_Settings->Get(key.c_str(), value).intValue = value;
}

static void SetValue(std::string& key, float value)
{
	Funkin::Settings::s_Settings->Get(key.c_str(), value).floatValue = value;
}

Funkin::JavosApp::JavosApp(Stratum::ApplicationInfo appInfo) : Stratum::Application(appInfo)
{
}

void Funkin::JavosApp::OnEarlyInit()
{
	Stratum::EventBus::RegisterListener<Stratum::ASInitializeEvent>([](Stratum::ASInitializeEvent e) {
		if (e.stage == "pre") {
			asIScriptEngine* engine = e.engine;
			engine->SetDefaultNamespace("Funkin");

			engine->RegisterGlobalFunction("bool getSettingsValue(const string &in key, bool def)", asFUNCTIONPR(GetValue, (std::string&, bool), bool), asCALL_CDECL);
			engine->RegisterGlobalFunction("int getSettingsValue(const string &in key, int def)", asFUNCTIONPR(GetValue, (std::string&, int), int), asCALL_CDECL);
			engine->RegisterGlobalFunction("float getSettingsValue(const string &in key, float def)", asFUNCTIONPR(GetValue, (std::string&, float), float), asCALL_CDECL);
			engine->RegisterGlobalFunction("void setSettingsValue(const string &in key, bool value)", asFUNCTIONPR(SetValue, (std::string&, bool), void), asCALL_CDECL);
			engine->RegisterGlobalFunction("void setSettingsValue(const string &in key, int value)", asFUNCTIONPR(SetValue, (std::string&, int), void), asCALL_CDECL);
			engine->RegisterGlobalFunction("void setSettingsValue(const string &in key, float value)", asFUNCTIONPR(SetValue, (std::string&, float), void), asCALL_CDECL);

			engine->SetDefaultNamespace("");
		}
	}, Stratum::EF_NONE);
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
