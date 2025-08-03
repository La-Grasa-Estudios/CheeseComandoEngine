#include "NVRHI_Vulkan.h"

#include <glm/glm.hpp>

using namespace ENGINE_NAMESPACE;

struct VkContextData
{
	glm::ivec2 WindowSize;
	uint32_t gFrameCount = 0;
};

void Render::BackendInitializerVulkan::InitializeBackend(Internal::Window* pWindow, RendererContext* pContext)
{
}

void Render::BackendInitializerVulkan::TerminateBackend(RendererContext* pContext)
{
}

void Render::BackendInitializerVulkan::BeginFrame()
{
}

void Render::BackendInitializerVulkan::Present(Internal::Window* pWindow, RendererContext* pContext)
{
}

bool Render::BackendInitializerVulkan::RequiresResize(Internal::Window* pWindow)
{
	return false;
}

void Render::BackendInitializerVulkan::ImGuiInit(Internal::Window* window)
{
}

void Render::BackendInitializerVulkan::ImGuiBeginFrame()
{
}

void Render::BackendInitializerVulkan::ImGuiEndFrame(RendererContext* pContext)
{
}

void Render::BackendInitializerVulkan::ImGuiShutdown()
{
}
