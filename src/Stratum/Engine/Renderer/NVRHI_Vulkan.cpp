#include "NVRHI_Vulkan.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <nvrhi/vulkan.h>
#include <nvrhi/validation.h>

#include <glm/glm.hpp>
#include <Core/Logger.h>
#include <Core/Window.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_syswm.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

using namespace ENGINE_NAMESPACE;

struct VkContextData
{
	glm::ivec2 WindowSize;
	uint32_t gFrameCount = 0;

	VkInstance instance;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties physicalMemoryProperties;

	Render::GraphicsDeviceProperties gdProperties;

	VkDevice device;
	VkSwapchainKHR swapChain;

	VkQueue presentQueue;
	VkQueue copyQueue;

	VkCommandBuffer setupCmdBuffer;

	uint32_t presentQueueIdx;
	uint32_t transferQueueIdx;

	VkImage* presentImages;
	VkDebugUtilsMessengerEXT callback;
};

VkContextData context;
PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessenger = NULL;
PFN_vkCreateWin32SurfaceKHR vkCreateWin32Surface = NULL;

void checkVulkanResult(VkResult result, const char* msg) {
#ifndef _DEBUG
	if (result != VK_SUCCESS)
	{
		Z_ERROR("Vulkan validation error: {}", msg);
	}
#endif
	assert(result == VK_SUCCESS, msg);
}

VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severityFlags, VkDebugUtilsMessageTypeFlagsEXT typeFlags, const VkDebugUtilsMessengerCallbackDataEXT* pData, void* pUserData) {

	OutputDebugStringA(pData->pMessageIdName);
	OutputDebugStringA(": ");
	OutputDebugStringA(pData->pMessage);
	OutputDebugStringA("\n");

	Z_ERROR("{}: {}", pData->pMessageIdName, pData->pMessage);

	if (severityFlags >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		assert(0);
	}

	return VK_FALSE;
}

VkSemaphore presentCompleteSemaphore[Render::MaxInFlightFrames];
VkSemaphore renderCompleteSemaphore[Render::MaxInFlightFrames];

void Render::BackendInitializerVulkan::InitializeBackend(Internal::Window* pWindow, RendererContext* pContext)
{
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // sType is a member of all structs
	applicationInfo.pNext = NULL;                               // as is pNext and flag
	applicationInfo.pApplicationName = "Engine BackEnd";            // The name of our application
	applicationInfo.pEngineName = "Stratum Engine";                         // The name of the engine
	applicationInfo.engineVersion = 1;                          // The version of the engine
	applicationInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);      // The version of Vulkan we're using

	VkInstanceCreateInfo instanceInfo = { };
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &applicationInfo;
	instanceInfo.enabledLayerCount = 0;
	instanceInfo.ppEnabledLayerNames = NULL;
	instanceInfo.enabledExtensionCount = 0;
	instanceInfo.ppEnabledExtensionNames = NULL;

	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);

	assert(layerCount != 0, "Failed to find any layer in your system.");

	VkLayerProperties* layersAvailable = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable);

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
	VkExtensionProperties* extensionsAvailable = new VkExtensionProperties[extensionCount];
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionsAvailable);

	std::vector<const char*> requestedExtensions = { "VK_KHR_surface", "VK_KHR_win32_surface" };
	std::vector<const char*> deviceExtensions = {};

#ifdef _DEBUG
	bool foundValidation = false;
	for (int i = 0; i < layerCount; ++i) {
		if (strcmp(layersAvailable[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
			foundValidation = true;
		}
	}
	if (foundValidation)
	{
		const char* layers[] = { "VK_LAYER_KHRONOS_validation" };

		requestedExtensions.push_back("VK_EXT_debug_utils");

		instanceInfo.enabledLayerCount = 1;
		instanceInfo.ppEnabledLayerNames = layers;
	}
	else
	{
		Z_WARN("[VULKAN] Cannot find VK_LAYER_LUNARG_standard_validation layer, validation layers will not be enabled.");
	}
#endif

	uint32_t numberRequiredExtensions = requestedExtensions.size();
	uint32_t foundExtensions = 0;
	for (uint32_t i = 0; i < extensionCount; ++i) {
		for (uint32_t j = 0; j < numberRequiredExtensions; ++j) {
			if (strcmp(extensionsAvailable[i].extensionName, requestedExtensions[j]) == 0) {
				foundExtensions++;
			}
		}
	}

	if (foundExtensions != numberRequiredExtensions)
	{
		Z_INFO("Could not find required extensions");
		for (auto s : requestedExtensions)
		{
			Z_INFO(s);
		}
	}

	instanceInfo.enabledExtensionCount = (uint32_t)requestedExtensions.size();
	instanceInfo.ppEnabledExtensionNames = requestedExtensions.data();

	SDL_SysWMinfo wmInfo{};
	SDL_version sdlver;
	SDL_VERSION(&sdlver);
	wmInfo.version = SDL_GetVersion(&sdlver);
	if (SDL_GetWindowWMInfo(pWindow->GetHandle(), &wmInfo, SDL_SYSWM_CURRENT_VERSION) != 0) {
		printf("Cant get native window handle: %s\n", SDL_GetError());
	}
	auto hWnd = wmInfo.info.win.window;

	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = wmInfo.info.win.hinstance;
	surfaceCreateInfo.hwnd = hWnd;

	checkVulkanResult(vkCreateInstance(&instanceInfo, NULL, &context.instance), "Failed to create vulkan instance.");

	VULKAN_HPP_DEFAULT_DISPATCHER.init(context.instance, vkGetInstanceProcAddr);

	*(void**)&vkCreateDebugUtilsMessenger = vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
	*(void**)&vkCreateWin32Surface = vkGetInstanceProcAddr(context.instance, "vkCreateWin32SurfaceKHR");

	VkDebugUtilsMessengerCreateInfoEXT callbackCreateInfo = {};
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	callbackCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	callbackCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	callbackCreateInfo.pfnUserCallback = &MyDebugReportCallback;

#ifdef _DEBUG
	auto result = vkCreateDebugUtilsMessenger(context.instance, &callbackCreateInfo, NULL, &context.callback);
#endif

	checkVulkanResult(vkCreateWin32Surface(context.instance, &surfaceCreateInfo, NULL, &context.surface), "Could not create surface.");

	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, NULL);
	VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
	vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, physicalDevices);

	std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> dedicatedDevices;
	std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> integratedDevices;

	for (uint32_t i = 0; i < physicalDeviceCount; ++i) {

		VkPhysicalDeviceProperties deviceProperties = {};
		vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, NULL);
		VkQueueFamilyProperties* queueFamilyProperties = new VkQueueFamilyProperties[queueFamilyCount];
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i],
			&queueFamilyCount,
			queueFamilyProperties);

		uint32_t queuesSupported = 0;
		uint32_t presentIdx = 0;
		uint32_t transferIdx = 0;

		for (uint32_t j = 0; j < queueFamilyCount; ++j) {

			VkBool32 supportsPresent;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], j, context.surface,
				&supportsPresent);

			if (supportsPresent && (queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !context.physicalDevice) {
				queuesSupported++;
				presentIdx = j;
			}

			if (queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				queuesSupported++;
				transferIdx = j;
			}
		}

		if (queuesSupported >= 2)
		{
			if (deviceProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				dedicatedDevices.push_back({ i, presentIdx, transferIdx });
			}
			if (deviceProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			{
				integratedDevices.push_back({ i, presentIdx, transferIdx });
			}
		}

		delete[] queueFamilyProperties;
	}

	if (!dedicatedDevices.empty())
	{
		auto device = dedicatedDevices[0];
		VkPhysicalDeviceProperties deviceProperties = {};
		vkGetPhysicalDeviceProperties(physicalDevices[std::get<0>(device)], &deviceProperties);
		context.physicalDevice = physicalDevices[std::get<0>(device)];
		context.physicalDeviceProperties = deviceProperties;
		context.presentQueueIdx = std::get<1>(device);
		context.transferQueueIdx = std::get<2>(device);
	}

	if (!context.physicalDevice && !integratedDevices.empty())
	{
		auto device = integratedDevices[0];
		VkPhysicalDeviceProperties deviceProperties = {};
		vkGetPhysicalDeviceProperties(physicalDevices[std::get<0>(device)], &deviceProperties);
		context.physicalDevice = physicalDevices[std::get<0>(device)];
		context.physicalDeviceProperties = deviceProperties;
		context.presentQueueIdx = std::get<1>(device);
		context.transferQueueIdx = std::get<2>(device);
	}

	delete[] physicalDevices;

	if (!context.physicalDevice) {
		Z_ERROR("Could not find a suitable physical device for Vulkan.");
		MessageBoxW(NULL, L"A graphics card with Vulkan 1.3 is required!", L"Incompatible graphics hardware!", MB_OK);
		return;
	}

	vkGetPhysicalDeviceMemoryProperties(context.physicalDevice, &context.physicalMemoryProperties);

	for (uint32_t i = 0; i < context.physicalMemoryProperties.memoryHeapCount; i++)
	{
		auto& heap = context.physicalMemoryProperties.memoryHeaps[i];
		if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
		{
			context.gdProperties.DedicatedVideoMemory = glm::max(context.gdProperties.DedicatedVideoMemory, heap.size);
		}
	}

	deviceExtensions.push_back("VK_KHR_swapchain");
	deviceExtensions.push_back("VK_KHR_synchronization2");

	// info for accessing one of the devices rendering queues:
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = context.presentQueueIdx;
	queueCreateInfo.queueCount = 1;
	float queuePriorities[] = { 1.0f };   // ask for highest priority for our queue. (range [0,1])
	queueCreateInfo.pQueuePriorities = queuePriorities;

	VkDeviceQueueCreateInfo queueCreateInfoTransfer = {};
	queueCreateInfoTransfer.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfoTransfer.queueFamilyIndex = context.transferQueueIdx;
	queueCreateInfoTransfer.queueCount = 1;
	queueCreateInfoTransfer.pQueuePriorities = queuePriorities;

	VkDeviceQueueCreateInfo infos[2] = { queueCreateInfo, queueCreateInfoTransfer };

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 2;
	deviceInfo.pQueueCreateInfos = infos;
	deviceInfo.enabledLayerCount = instanceInfo.enabledLayerCount;
	deviceInfo.ppEnabledLayerNames = instanceInfo.ppEnabledLayerNames;

	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

	VkPhysicalDeviceFeatures features = {};
	features.shaderClipDistance = VK_TRUE;
	features.samplerAnisotropy = true;
	deviceInfo.pEnabledFeatures = &features;

	vk::PhysicalDeviceVulkan13Features features13;
	features13.synchronization2 = true;
	features13.shaderDemoteToHelperInvocation = true;

	vk::PhysicalDeviceVulkan12Features features12;
	features12.timelineSemaphore = true;
	features12.runtimeDescriptorArray = true;
	features12.shaderSampledImageArrayNonUniformIndexing = true;
	features12.descriptorBindingPartiallyBound = VK_TRUE;
	features12.pNext = &features13;

	deviceInfo.pNext = &features12;

	checkVulkanResult(vkCreateDevice(context.physicalDevice, &deviceInfo, NULL, &context.device), "Failed to create logical device!");

	if (!context.device) {
		Z_ERROR("Could not create the logical device for Vulkan.");
		Z_ERROR("Trying to use {}", context.physicalDeviceProperties.deviceName);
		MessageBoxW(NULL, L"Error: Task completed succesfully", L"Incompatible graphics hardware!", MB_OK);
		return;
	}

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface,
		&formatCount, NULL);
	VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[formatCount];
	vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface,
		&formatCount, surfaceFormats);

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface has
	// no preferred format. Otherwise, at least one supported format will be returned.
	VkFormat colorFormat;
	if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		colorFormat = VK_FORMAT_B8G8R8_UNORM;
	}
	else {
		colorFormat = surfaceFormats[0].format;
	}
	VkColorSpaceKHR colorSpace;
	colorSpace = surfaceFormats[0].colorSpace;
	delete[] surfaceFormats;

	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface,
		&surfaceCapabilities);

	// we are effectively looking for double-buffering:
	// if surfaceCapabilities.maxImageCount == 0 there is actually no limit on the number of images! 
	uint32_t desiredImageCount = Render::MaxInFlightFrames;
	if (desiredImageCount < surfaceCapabilities.minImageCount) {
		desiredImageCount = surfaceCapabilities.minImageCount;
	}
	else if (surfaceCapabilities.maxImageCount != 0 &&
		desiredImageCount > surfaceCapabilities.maxImageCount) {
		desiredImageCount = surfaceCapabilities.maxImageCount;
	}

	VkExtent2D surfaceResolution = surfaceCapabilities.currentExtent;
	if (surfaceResolution.width == -1) {
		surfaceResolution.width = context.WindowSize.x;
		surfaceResolution.height = context.WindowSize.y;
	}
	else {
		context.WindowSize.x = surfaceResolution.width;
		context.WindowSize.y = surfaceResolution.height;
	}

	VkSurfaceTransformFlagBitsKHR preTransform = surfaceCapabilities.currentTransform;
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface,
		&presentModeCount, NULL);
	VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
	vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface,
		&presentModeCount, presentModes);

	VkPresentModeKHR presentationMode = VK_PRESENT_MODE_FIFO_KHR;   // always supported.
	for (uint32_t i = 0; i < presentModeCount; ++i) {
		if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentationMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			break;
		}
	}
	delete[] presentModes;

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = context.surface;
	swapChainCreateInfo.minImageCount = desiredImageCount;
	swapChainCreateInfo.imageFormat = colorFormat;
	swapChainCreateInfo.imageColorSpace = colorSpace;
	swapChainCreateInfo.imageExtent = surfaceResolution;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;   // <--
	swapChainCreateInfo.preTransform = preTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentationMode;
	swapChainCreateInfo.clipped = true;     // If we want clipping outside the extents
	// (remember our device features?)

	checkVulkanResult(vkCreateSwapchainKHR(context.device, &swapChainCreateInfo, NULL, &context.swapChain), "Failed to create swapchain.");

	vkGetDeviceQueue(context.device, context.presentQueueIdx, 0, &context.presentQueue);
	vkGetDeviceQueue(context.device, context.transferQueueIdx, 0, &context.copyQueue);

	// create our command buffers:
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = context.presentQueueIdx;

	VkCommandPool commandPool;
	checkVulkanResult(vkCreateCommandPool(context.device, &commandPoolCreateInfo, NULL, &commandPool), "Failed to create command pool.");

	VkCommandBufferAllocateInfo commandBufferAllocationInfo = {};
	commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocationInfo.commandPool = commandPool;
	commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocationInfo.commandBufferCount = 1;

	checkVulkanResult(vkAllocateCommandBuffers(context.device, &commandBufferAllocationInfo, &context.setupCmdBuffer), "Failed to allocate setup command buffer.");

	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, NULL);
	context.presentImages = new VkImage[imageCount];    // this should be 2 for double-buffering
	vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, context.presentImages);

	VkImageViewCreateInfo presentImagesViewCreateInfo = {};
	presentImagesViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	presentImagesViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	presentImagesViewCreateInfo.format = colorFormat;
	presentImagesViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	presentImagesViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentImagesViewCreateInfo.subresourceRange.baseMipLevel = 0;
	presentImagesViewCreateInfo.subresourceRange.levelCount = 1;
	presentImagesViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	presentImagesViewCreateInfo.subresourceRange.layerCount = 1;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence submitFence;
	vkCreateFence(context.device, &fenceCreateInfo, NULL, &submitFence);

	bool* transitioned = new bool[imageCount];
	memset(transitioned, 0, sizeof(bool) * imageCount);
	uint32_t doneCount = 0;
	while (doneCount != imageCount) {

		VkSemaphore presentCompleteSemaphore;
		VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0 };
		vkCreateSemaphore(context.device, &semaphoreCreateInfo, NULL, &presentCompleteSemaphore);

		uint32_t nextImageIdx;
		vkAcquireNextImageKHR(context.device, context.swapChain, UINT64_MAX,
			presentCompleteSemaphore, VK_NULL_HANDLE, &nextImageIdx);

		if (!transitioned[nextImageIdx]) {

			// start recording out image layout change barrier on our setup command buffer:
			vkBeginCommandBuffer(context.setupCmdBuffer, &beginInfo);

			VkImageMemoryBarrier layoutTransitionBarrier = {};
			layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			layoutTransitionBarrier.srcAccessMask = 0;
			layoutTransitionBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			layoutTransitionBarrier.image = context.presentImages[nextImageIdx];
			VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			layoutTransitionBarrier.subresourceRange = resourceRange;

			vkCmdPipelineBarrier(context.setupCmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0,
				0, NULL,
				0, NULL,
				1, &layoutTransitionBarrier);

			vkEndCommandBuffer(context.setupCmdBuffer);

			VkPipelineStageFlags waitStageMash[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
			submitInfo.pWaitDstStageMask = waitStageMash;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &context.setupCmdBuffer;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = NULL;
			vkQueueSubmit(context.presentQueue, 1, &submitInfo, submitFence);

			vkWaitForFences(context.device, 1, &submitFence, VK_TRUE, UINT64_MAX);
			vkResetFences(context.device, 1, &submitFence);

			vkDestroySemaphore(context.device, presentCompleteSemaphore, NULL);

			vkResetCommandBuffer(context.setupCmdBuffer, 0);

			transitioned[nextImageIdx] = true;
			doneCount++;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 0;
		presentInfo.pWaitSemaphores = NULL;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &context.swapChain;
		presentInfo.pImageIndices = &nextImageIdx;
		vkQueuePresentKHR(context.presentQueue, &presentInfo);
	}
	delete[] transitioned;

	nvrhi::vulkan::DeviceDesc deviceDesc;
	deviceDesc.errorCB = &pContext->mCallbackLogger;
	deviceDesc.physicalDevice = context.physicalDevice;
	deviceDesc.device = context.device;
	deviceDesc.transferQueue = context.copyQueue;
	deviceDesc.transferQueueIndex = context.transferQueueIdx;
	deviceDesc.graphicsQueue = context.presentQueue;
	deviceDesc.graphicsQueueIndex = context.presentQueueIdx;
	deviceDesc.deviceExtensions = deviceExtensions.data();
	deviceDesc.numDeviceExtensions = deviceExtensions.size();
	deviceDesc.instance = context.instance;

	pContext->pDevice = nvrhi::vulkan::createDevice(deviceDesc);

#ifdef _DEBUG
	pContext->pDevice = nvrhi::validation::createValidationLayer(pContext->pDevice);
#endif

	mCommandList = pContext->pDevice->createCommandList();

	for (uint32_t i = 0; i < desiredImageCount; ++i)
	{
		auto textureDesc = nvrhi::TextureDesc()
			.setDimension(nvrhi::TextureDimension::Texture2D)
			.setFormat(nvrhi::Format::BGRA8_UNORM)
			.setWidth(context.WindowSize.x)
			.setHeight(context.WindowSize.y)
			.setIsRenderTarget(true)
			.setInitialState(nvrhi::ResourceStates::Present)
			.setKeepInitialState(false)
			.setDebugName("Swap Chain Image");

		pContext->NvBackBuffers[i] = pContext->pDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, context.presentImages[i], textureDesc);

		VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0 };
		vkCreateSemaphore(context.device, &semaphoreCreateInfo, NULL, &presentCompleteSemaphore[i]);
		vkCreateSemaphore(context.device, &semaphoreCreateInfo, NULL, &renderCompleteSemaphore[i]);

	}
	
	pContext->FrameIndex = 0;
}

void Render::BackendInitializerVulkan::TerminateBackend(RendererContext* pContext)
{
}

uint32_t frameIndex = 0;

void Render::BackendInitializerVulkan::BeginFrame()
{
	vkAcquireNextImageKHR(context.device, context.swapChain, UINT64_MAX,
		presentCompleteSemaphore[frameIndex], VK_NULL_HANDLE, &frameIndex);
}

namespace nvrhi
{
	namespace vulkan
	{
		extern std::mutex g_QueueLock;
	}
}

void Render::BackendInitializerVulkan::Present(Internal::Window* pWindow, RendererContext* pContext)
{
	nvrhi::vulkan::g_QueueLock.lock();

	VkFence renderFence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(context.device, &fenceCreateInfo, NULL, &renderFence);

	VkPipelineStageFlags waitStageMash = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &presentCompleteSemaphore[frameIndex];
	submitInfo.pWaitDstStageMask = &waitStageMash;
	submitInfo.commandBufferCount = 0;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore[frameIndex];
	vkQueueSubmit(context.presentQueue, 1, &submitInfo, renderFence);

	vkWaitForFences(context.device, 1, &renderFence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(context.device, renderFence, NULL);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderCompleteSemaphore[frameIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &context.swapChain;
	presentInfo.pImageIndices = &pContext->FrameIndex;
	presentInfo.pResults = NULL;
	vkQueuePresentKHR(context.presentQueue, &presentInfo);

	if (RequiresResize(pWindow))
	{
		context.WindowSize = pWindow->GetFramebuffer()->GetSize();

		vkDeviceWaitIdle(context.device);
		vkQueueWaitIdle(context.presentQueue);
		vkQueueWaitIdle(context.copyQueue);

		pContext->pDevice->waitForIdle();
		pContext->pDevice->runGarbageCollection();

		for (int i = 0; i < Render::MaxInFlightFrames; i++)
		{
			pContext->NvFramebufferRtvs[i] = nullptr;
			pContext->NvBackBuffers[i] = nullptr;
		}

		pContext->pDevice->waitForIdle();
		pContext->pDevice->runGarbageCollection();

		vkDestroySwapchainKHR(context.device, context.swapChain, nullptr);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface,
			&formatCount, NULL);
		VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[formatCount];
		vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface,
			&formatCount, surfaceFormats);

		// If the format list includes just one entry of VK_FORMAT_UNDEFINED, the surface has
		// no preferred format. Otherwise, at least one supported format will be returned.
		VkFormat colorFormat;
		if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
			colorFormat = VK_FORMAT_B8G8R8_UNORM;
		}
		else {
			colorFormat = surfaceFormats[0].format;
		}
		VkColorSpaceKHR colorSpace;
		colorSpace = surfaceFormats[0].colorSpace;
		delete[] surfaceFormats;

		VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface,
			&surfaceCapabilities);

		// we are effectively looking for double-buffering:
		// if surfaceCapabilities.maxImageCount == 0 there is actually no limit on the number of images! 
		uint32_t desiredImageCount = Render::MaxInFlightFrames;
		if (desiredImageCount < surfaceCapabilities.minImageCount) {
			desiredImageCount = surfaceCapabilities.minImageCount;
		}
		else if (surfaceCapabilities.maxImageCount != 0 &&
			desiredImageCount > surfaceCapabilities.maxImageCount) {
			desiredImageCount = surfaceCapabilities.maxImageCount;
		}

		VkExtent2D surfaceResolution = surfaceCapabilities.currentExtent;
		surfaceResolution.width = context.WindowSize.x;
		surfaceResolution.height = context.WindowSize.y;

		VkSurfaceTransformFlagBitsKHR preTransform = surfaceCapabilities.currentTransform;
		if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface,
			&presentModeCount, NULL);
		VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
		vkGetPhysicalDeviceSurfacePresentModesKHR(context.physicalDevice, context.surface,
			&presentModeCount, presentModes);

		VkPresentModeKHR presentationMode = VK_PRESENT_MODE_FIFO_KHR;   // always supported.
		for (uint32_t i = 0; i < presentModeCount; ++i) {
			if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				presentationMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				break;
			}
		}
		delete[] presentModes;

		VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = context.surface;
		swapChainCreateInfo.minImageCount = desiredImageCount;
		swapChainCreateInfo.imageFormat = colorFormat;
		swapChainCreateInfo.imageColorSpace = colorSpace;
		swapChainCreateInfo.imageExtent = surfaceResolution;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;   // <--
		swapChainCreateInfo.preTransform = preTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = presentationMode;
		swapChainCreateInfo.clipped = true;     // If we want clipping outside the extents
		// (remember our device features?)

		checkVulkanResult(vkCreateSwapchainKHR(context.device, &swapChainCreateInfo, NULL, &context.swapChain), "Failed to create swapchain.");

		VkImageViewCreateInfo presentImagesViewCreateInfo = {};
		presentImagesViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		presentImagesViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		presentImagesViewCreateInfo.format = colorFormat;
		presentImagesViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		presentImagesViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		presentImagesViewCreateInfo.subresourceRange.baseMipLevel = 0;
		presentImagesViewCreateInfo.subresourceRange.levelCount = 1;
		presentImagesViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		presentImagesViewCreateInfo.subresourceRange.layerCount = 1;

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence submitFence;
		vkCreateFence(context.device, &fenceCreateInfo, NULL, &submitFence);

		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, NULL);
		vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, context.presentImages);

		bool* transitioned = new bool[imageCount];
		memset(transitioned, 0, sizeof(bool) * imageCount);
		uint32_t doneCount = 0;
		while (doneCount != imageCount) {

			VkSemaphore presentCompleteSemaphore;
			VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0 };
			vkCreateSemaphore(context.device, &semaphoreCreateInfo, NULL, &presentCompleteSemaphore);

			uint32_t nextImageIdx;
			vkAcquireNextImageKHR(context.device, context.swapChain, UINT64_MAX,
				presentCompleteSemaphore, VK_NULL_HANDLE, &nextImageIdx);

			if (!transitioned[nextImageIdx]) {

				// start recording out image layout change barrier on our setup command buffer:
				vkBeginCommandBuffer(context.setupCmdBuffer, &beginInfo);

				VkImageMemoryBarrier layoutTransitionBarrier = {};
				layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				layoutTransitionBarrier.srcAccessMask = 0;
				layoutTransitionBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				layoutTransitionBarrier.image = context.presentImages[nextImageIdx];
				VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				layoutTransitionBarrier.subresourceRange = resourceRange;

				vkCmdPipelineBarrier(context.setupCmdBuffer,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					0,
					0, NULL,
					0, NULL,
					1, &layoutTransitionBarrier);

				vkEndCommandBuffer(context.setupCmdBuffer);

				VkPipelineStageFlags waitStageMash[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
				submitInfo.pWaitDstStageMask = waitStageMash;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &context.setupCmdBuffer;
				submitInfo.signalSemaphoreCount = 0;
				submitInfo.pSignalSemaphores = NULL;
				vkQueueSubmit(context.presentQueue, 1, &submitInfo, submitFence);

				vkWaitForFences(context.device, 1, &submitFence, VK_TRUE, UINT64_MAX);
				vkResetFences(context.device, 1, &submitFence);

				vkDestroySemaphore(context.device, presentCompleteSemaphore, NULL);

				vkResetCommandBuffer(context.setupCmdBuffer, 0);

				transitioned[nextImageIdx] = true;
				doneCount++;
				pContext->pDevice->waitForIdle();
			}

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 0;
			presentInfo.pWaitSemaphores = NULL;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &context.swapChain;
			presentInfo.pImageIndices = &nextImageIdx;
			vkQueuePresentKHR(context.presentQueue, &presentInfo);
		}
		delete[] transitioned;

		pContext->pDevice->waitForIdle();

		for (uint32_t i = 0; i < desiredImageCount; ++i)
		{
			auto textureDesc = nvrhi::TextureDesc()
				.setDimension(nvrhi::TextureDimension::Texture2D)
				.setFormat(nvrhi::Format::BGRA8_UNORM)
				.setWidth(context.WindowSize.x)
				.setHeight(context.WindowSize.y)
				.setIsRenderTarget(true)
				.setInitialState(nvrhi::ResourceStates::Present)
				.setKeepInitialState(false)
				.setDebugName("Swap Chain Image");

			pContext->NvBackBuffers[i] = pContext->pDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, context.presentImages[i], textureDesc);
			nvrhi::vulkan::g_QueueLock.unlock();
			mCommandList->open();
			mCommandList->setEnableAutomaticBarriers(false);
			mCommandList->beginTrackingTextureState(pContext->NvBackBuffers[i], nvrhi::AllSubresources, nvrhi::ResourceStates::Unknown);
			mCommandList->setTextureState(pContext->NvBackBuffers[i], nvrhi::AllSubresources, nvrhi::ResourceStates::Present);
			mCommandList->close();
			pContext->pDevice->executeCommandList(mCommandList);
			pContext->pDevice->waitForIdle();
			nvrhi::vulkan::g_QueueLock.lock();
		}

		frameIndex = Render::MaxInFlightFrames - 1;
	}

	frameIndex = (frameIndex + 1) % Render::MaxInFlightFrames;
	pContext->FrameIndex = frameIndex;

	nvrhi::vulkan::g_QueueLock.unlock();

}

bool Render::BackendInitializerVulkan::RequiresResize(Internal::Window* pWindow)
{
	glm::ivec2 size = pWindow->GetFramebuffer()->GetSize();
	return size != context.WindowSize;
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

Render::GraphicsDeviceProperties Render::BackendInitializerVulkan::GetGraphicsDeviceProperties()
{
	GraphicsDeviceProperties device{};
	device.Description = context.physicalDeviceProperties.deviceName;
	device.DedicatedVideoMemory = context.gdProperties.DedicatedVideoMemory;
	return device;
}
