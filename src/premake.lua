require 'modules/android_studio/android_studio'

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
	includedirs { "../Dependencies/Include",
	"../Dependencies/Thirdparty/angelscript/sdk/angelscript/include",
	"../Dependencies/Thirdparty/angelscript/sdk/add_on",
	"Stratum/Engine" }
end

function stratum_config() --every stratum project calls this
	base_config()
	links { "Stratum-Core", "angelscript" }
end
	
filter "toolset:msc*"
    buildoptions { "/MP" }
	defines { "_CRT_SECURE_NO_WARNINGS" }
	
function stratum_core()
	base_config()
	kind "StaticLib"
	links { "angelscript" }
	
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
end
   
project "Stratum-Core"
	stratum_core()
	
group "Apps"	

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
	
project "3D"
	stratum_config()
	kind "ConsoleApp"
	
	files {
		"3D/**.c",
		"3D/**.cpp",
		"3D/**.h*",
		"3D/**.hpp"
	}

	filter "configurations:Debug"
	defines { "_DEBUG" }
	symbols "On"
	
	filter "configurations:Release"
	defines { "NDEBUG" }
	optimize "On"
	
group "Tools"		
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
	
group "ThirdParty"
include "angelscript"