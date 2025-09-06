
# Stratum Engine

A toy game engine made in C++ with DirectX 12 and (soon™) Vulkan, currently it's main goal is 2d rendering for a project i'm making, next is 3d rendering.
Goals:
+ Finish the 2d project
+ Add 3d rendering again
+ Clustered Deferred rendering
+ Async Compute

## Building

### Windows

Just run generate-vs2022.bat, premake should generate project files in the "projects/" directory and the solution file in the root directory

### Linux

¯\\_(ツ)__/¯

## Run

Requires atleast a GPU with DirectX 12 drivers, feature level 11.0, shader model 6.5 and resource binding tier 2.  
Requires atleast a GPU with Vulkan 1.3, VK_KHR_surface, (Windows) VK_KHR_win32_surface, VK_KHR_swapchain & VK_KHR_synchronization2

## Dependencies

 - [NVRHI](https://github.com/NVIDIA-RTX/NVRHI)
 - [SDL](https://github.com/libsdl-org/SDL)
 - [Freetype](https://github.com/freetype/freetype)
 - [Zlib](https://github.com/madler/zlib)
 - [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler/releases)
 - [LibAvCodec](https://github.com/libav/libav)
 - [GLM](https://github.com/g-truc/glm)
 - [nlohmann JSON](https://github.com/nlohmann/json)


## License

[GNU AGPLV3](https://choosealicense.com/licenses/agpl-3.0/)

### Authors of the original mod
https://www.youtube.com/watch?v=-TZ2dkwWjTc  
[The mod from where the UI textures are](https://gamebanana.com/mods/456005)

