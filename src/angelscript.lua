project "angelscript"
	base_config()
	kind "StaticLib"
	
	files {
		"../Dependencies/Thirdparty/angelscript/sdk/angelscript/source/as_callfunc_x64_msvc_asm.asm",
		"../Dependencies/Thirdparty/angelscript/sdk/angelscript/source/**.cpp",
		"../Dependencies/Thirdparty/angelscript/sdk/angelscript/source/**.h",
		"../Dependencies/Thirdparty/angelscript/sdk/add_on/**.cpp",
		"../Dependencies/Thirdparty/angelscript/sdk/add_on/**.h",
	}
	
	filter "system:windows"
        systemversion "latest"
        defines { "_WINDOWS", "AS_NO_THREADS" } -- optional flags

	filter "configurations:Debug"
	defines { "_DEBUG", "ANGELSCRIPT_EXPORT", "_LIB", "AS_DEBUG" }
	symbols "On"
	
	filter "configurations:Release"
	defines { "NDEBUG", "_LIB", "ANGELSCRIPT_EXPORT"  }
	optimize "On"	