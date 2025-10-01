#include "AngelScript.h"

#include <Core/Logger.h>
#include <Core/Time.h>
#include <Event/EventBus.h>
#include <VFS/ZVFS.h>

#include <angelscript.h>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptbuilder/scriptbuilder.h>
#include <cassert>

using namespace ENGINE_NAMESPACE;

// From AS github
extern void GenerateScriptPredefined(const asIScriptEngine* engine, const std::string& path);

static void ENGINE_AS_CALL MessageCallback(const asSMessageInfo* msg, void* param)
{
	const char* type = "ERR ";
	if (msg->type == asMSGTYPE_WARNING)
		type = "WARN";
	else if (msg->type == asMSGTYPE_INFORMATION)
		type = "INFO";
	Z_INFO("[AngelScript] {} ({}, {}) : {} : {}", msg->section, msg->row, msg->col, type, msg->message);
}

static int IncludeCallback(const char* include, const char* from, CScriptBuilder* builder, void* userParam)
{
	std::string path = include;
	if (path[0] != '/')
	{
		std::string base = from;
		size_t slash = base.find_last_of('/');
		if (slash != std::string::npos)
			base = base.substr(0, slash + 1);
		else
			base = "";
		path = base + path;
	}
	std::string code;
	if (!ZVFS::Exists(path.c_str()))
	{
		Z_ERROR("[AngelScript] Failed to open include file: {}", path);
		return -1;
	}
	else
	{
		code = ZVFS::GetFile(path.c_str())->Str();
	}
	builder->AddSectionFromMemory(path.c_str(), code.c_str(), code.size());
	return 0;
}

static void ENGINE_AS_CALL asWrapper_LogInfo(std::string msg)
{
	Z_INFO(msg);
}

AngelScriptEngine::AngelScriptEngine()
{
}

extern void RegisterMath(asIScriptEngine* engine);

void AngelScriptEngine::Init()
{
	asIScriptEngine* engine = asCreateScriptEngine();
	AS_RETURN_CHECK(engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL));

	RegisterStdString(engine);

	//engine->SetDefaultNamespace("Stratum");
	AS_RETURN_CHECK(engine->RegisterGlobalFunction("void logInfo(const string &in)", asFUNCTION(asWrapper_LogInfo), asCALL_CDECL));
	AS_RETURN_CHECK(engine->RegisterGlobalProperty("const float deltaTime", &Time::DeltaTime));
	AS_RETURN_CHECK(engine->RegisterGlobalProperty("float timeScale", &Time::TimeScale));
	
	RegisterMath(engine);

	EventBus::InvokeEvent(ASInitializeEvent{ "pre", engine });

	engine->SetDefaultNamespace("");

	mEngine = engine;

	GenerateScriptPredefined(mEngine, "Data/scripts/as.predefined");
	EventBus::InvokeEvent(ASInitializeEvent{ "post", engine });
}

asIScriptContext* AngelScriptEngine::CreateContext()
{
	asIScriptContext* ctx = mEngine->CreateContext();
	return ctx;
}

asIScriptContext* AngelScriptEngine::RunContext(const char* func, asIScriptModule* module)
{
	auto ctx = CreateContext();
	asIScriptFunction* funccall = module->GetFunctionByDecl(func);
	ctx->Prepare(funccall);
	int r = ctx->Execute();
	if (r != asEXECUTION_FINISHED)
	{
		// The execution didn't complete as expected. Determine what happened.
		if (r == asEXECUTION_EXCEPTION)
		{
			// An exception occurred, let the script writer know what happened so it can be corrected.
			printf("An exception '%s' occurred. Please correct the code and try again.\n", ctx->GetExceptionString());
			return nullptr;
		}
	}
	return ctx;
}

AngelScriptEngine& AngelScriptEngine::Get()
{
	static AngelScriptEngine instance;
	return instance;
}

asIScriptModule* AngelScriptEngine::BuildModule(const char* path, const char* name)
{
	std::string script = ZVFS::GetFile(path)->Str();

	CScriptBuilder builder;
	int r = builder.StartNewModule(mEngine, name);
	builder.SetIncludeCallback(IncludeCallback, nullptr);
	if (r < 0)
	{
		// If the code fails here it is usually because there
		// is no more memory to allocate the module
		printf("Unrecoverable error while starting a new module.\n");
		return nullptr;
	}
	r = builder.AddSectionFromMemory(path, script.c_str());
	if (r < 0)
	{
		// The builder wasn't able to load the file. Maybe the file
		// has been removed, or the wrong name was given, or some
		// preprocessing commands are incorrectly written.
		printf("Please correct the errors in the script and try again.\n");
		return nullptr;
	}
	r = builder.BuildModule();
	if (r < 0)
	{
		// An error occurred. Instruct the script writer to fix the 
		// compilation errors that were listed in the output stream.
		printf("Please correct the errors in the script and try again.\n");
		return nullptr;
	}

	return mEngine->GetModule(name);
}