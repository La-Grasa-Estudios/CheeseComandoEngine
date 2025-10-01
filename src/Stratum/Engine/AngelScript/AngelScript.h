#pragma once
#include <znmsp.h>

#define ENGINE_AS_CALL _cdecl
#define AS_RETURN_CHECK(x) { int r = x; assert(r >= 0); }

// AngelScript Typedefs
class asIScriptEngine;
class asIScriptModule;
class asIScriptContext;
class asIScriptGeneric;
class asIScriptObject;
class asITypeInfo;
class asIScriptFunction;
class asIBinaryStream;
class asIJITCompilerAbstract;
class asIThreadManager;
class asILockableSharedBool;
class asIStringFactory;

BEGIN_ENGINE

struct ASInitializeEvent
{
	const char* stage;
	asIScriptEngine* engine;
};

// Simple wrapper
class AngelScriptEngine
{
public:
	AngelScriptEngine();

	void Init();
	void Shutdown();

	asIScriptModule* BuildModule(const char* path, const char* name);
	asIScriptContext* CreateContext();
	// Runs the script one single time, useful for script that initializes things
	// Remember to call free on the context when finished
	// Returns null if there was an exception running the function
	asIScriptContext* RunContext(const char* func, asIScriptModule* module);

	asIScriptEngine* GetEngine() { return mEngine; }

	static AngelScriptEngine& Get();
private:
	asIScriptEngine* mEngine = nullptr;
};

END_ENGINE