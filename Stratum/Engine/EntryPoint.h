#pragma once

#include "Core/Application.h"

#include <thread>
#include <iostream>
#include <fstream>
#include <filesystem>

ENGINE_NAMESPACE::Application* g_CurrentApp;

extern ENGINE_NAMESPACE::Application* AppMain(std::vector<std::string> args);

char** g_argv;
int g_argc;

void AppThread() {
	std::vector<std::string> args;

	for (int i = 1; i < g_argc; i++)
	{
		std::string str = g_argv[i];
		args.push_back(str);
	}

	g_CurrentApp = AppMain(args);

	if (!g_CurrentApp) return;

	g_CurrentApp->Run(args);

	delete g_CurrentApp;
}

int main(int argc, char** argv) {
	g_argv = argv;
	g_argc = argc;

	std::filesystem::current_path("../");

	AppThread();

	return 0;
}

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char*, int nShowCmd)
{
	std::ofstream out("EngineLog.log");
	std::streambuf* coutbuf = std::cout.rdbuf(); //save old buf
	std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!

	return main(__argc, __argv);
}

#endif // _WIN32