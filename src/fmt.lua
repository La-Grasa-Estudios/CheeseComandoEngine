project "fmt"
	base_config()
	kind "StaticLib"
	
	files {
		"../Dependencies/Thirdparty/fmt/src/**.cc"
	}
	
	filter "toolset:msc*"
	buildoptions { "/utf-8" }

	filter "configurations:Debug"
	defines { "_DEBUG" }
	symbols "On"
	
	filter "configurations:Release"
	defines { "NDEBUG"  }
	optimize "On"	
	
-- Fuck it fmt can be header only lol