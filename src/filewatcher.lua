project "filewatcher"
	base_config()
	kind "StaticLib"
	
	files {
		"../Dependencies/Thirdparty/filewatcher/src/**.cpp"
	}

	filter "configurations:Debug"
	defines { "_DEBUG" }
	symbols "On"
	
	filter "configurations:Release"
	defines { "NDEBUG"  }
	optimize "On"	