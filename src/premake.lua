
workspace "Stratum Engine"
   configurations { "Debug", "Release" }
   platforms { "x64" }
   location "../"
   startproject("Project Javos")

require("vstudio")
premake.vstudio.solutionExplorerScope = "project"
	
function base_config()
	filter {}
	objdir ("../obj/%{prj.name}/%{cfg.platform}/%{cfg.buildcfg}")
	targetdir ("../%{cfg.platform}/%{cfg.buildcfg}/Bin")
	debugdir "../Build/Bin"
	libdirs { "../Dependencies/Lib" }
    cppdialect "C++20"
	location "../projects"
	language "C++"
	includedirs { "../Dependencies/Include", "Stratum/Engine" }
end

function stratum_config() --every stratum project calls this
	base_config()
	links { "Stratum-Core" }
end
	
filter "action:vs*"
    buildoptions { "/MP" }
	defines { "_CRT_SECURE_NO_WARNINGS" }
   
project "Stratum-Core"
	base_config()
	kind "StaticLib"
	
	files {
		"Stratum/Engine/**.c",
		"Stratum/Engine/**.cpp",
		"Stratum/Engine/**.h*",
		"Stratum/Engine/**.hpp"
	}

	filter "configurations:Debug"
	defines { "_DEBUG" }
	symbols "On"
	
	filter "configurations:Release"
	defines { "NDEBUG" }
	optimize "On"
	
project "Project Javos"
	stratum_config()
	
	files {
		"Project Javos/**.c",
		"Project Javos/**.cpp",
		"Project Javos/**.h*",
		"Project Javos/**.hpp"
	}

	filter "configurations:Debug"
	defines { "_DEBUG" }
	symbols "On"
	kind "ConsoleApp"
	
	filter "configurations:Release"
	defines { "NDEBUG" }
	optimize "On"
	kind "WindowedApp"
	
project "ResourceCompiler"

	stratum_config()
	links { "zlibstatic" }
	
	kind "ConsoleApp"
	
	files {
		"ResourceCompiler/**.c",
		"ResourceCompiler/**.cpp",
		"ResourceCompiler/**.h*",
		"ResourceCompiler/**.hpp"
	}

	filter "configurations:Debug"
	defines { "_DEBUG" }
	symbols "On"
	
	filter "configurations:Release"
	defines { "NDEBUG" }
	optimize "On"
	