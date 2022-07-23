#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define cast(type) (type)

#define VkCheck(result)                                        \
    do {                                                       \
        VkResult _result = result;                             \
        if (_result != VK_SUCCESS) {                           \
            fflush(stdout);                                    \
            fprintf(stderr, #result " failed! %x\n", _result); \
            exit(1);                                           \
        }                                                      \
    } while (0)

VkBool32 VKAPI_CALL DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                           VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                           void* pUserData) {
    const char* flagString = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)   ? "Verbose"
                             : (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    ? "Info"
                             : (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "Warning"
                             : (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   ? "Error"
                                                                                                   : "Unknown Type";
    (void)messageTypes;
    (void)pUserData;
    fflush(stdout);
    fprintf(stderr, "%s: %s\n", flagString, pCallbackData->pMessage);
    return VK_TRUE;
}

static bool Running = true;

LRESULT CALLBACK WindowMessageCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch (message) {
        case WM_CLOSE: {
            Running = false;
        } break;

        default: {
            result = DefWindowProcA(hWnd, message, wParam, lParam);
        } break;
    }
    return result;
}

int main() {
    const size_t WindowWidth          = 640;
    const size_t WindowHeight         = 480;
    const char* const WindowClassName = "Vulkan Testing";
    const char* const WindowTitle     = "Vulkan Testing";
    HINSTANCE hinstance               = GetModuleHandleA(NULL);
    HWND windowHandle                 = NULL;
    {
        const DWORD WindowStyle   = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
        const DWORD WindowStyleEx = 0;

        if (RegisterClassExA(&(WNDCLASSEXA){
                .cbSize        = sizeof(WNDCLASSEXA),
                .style         = CS_OWNDC,
                .lpfnWndProc   = WindowMessageCallback,
                .hInstance     = hinstance,
                .hCursor       = LoadCursor(NULL, IDC_ARROW),
                .lpszClassName = WindowClassName,
            }) == 0) {
            fflush(stdout);
            fprintf(stderr, "Failed to register a window class! %lx\n", GetLastError());
            exit(1);
        }

        RECT windowRect   = {};
        windowRect.left   = 100;
        windowRect.right  = windowRect.left + cast(LONG) WindowWidth;
        windowRect.top    = 100;
        windowRect.bottom = windowRect.left + cast(LONG) WindowHeight;
        if (!AdjustWindowRectEx(&windowRect, WindowStyle, false, WindowStyleEx)) {
            fflush(stdout);
            fprintf(stderr, "Failed to get window rect size! %lx\n", GetLastError());
            exit(1);
        }

        windowHandle = CreateWindowExA(WindowStyleEx,
                                       WindowClassName,
                                       WindowTitle,
                                       WindowStyle,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       windowRect.right - windowRect.left,
                                       windowRect.bottom - windowRect.top,
                                       NULL,
                                       NULL,
                                       hinstance,
                                       NULL);
        if (windowHandle == NULL) {
            fflush(stdout);
            fprintf(stderr, "Failed to create window! %lx\n", GetLastError());
            exit(1);
        }
    }

    VkAllocationCallbacks* allocator = NULL;

    const uint32_t vulkanVersion = VK_API_VERSION_1_2;
    {
        uint32_t actualVulkanVersion         = 0;
        const VkResult instanceVersionResult = vkEnumerateInstanceVersion(&actualVulkanVersion);
        if (instanceVersionResult != VK_SUCCESS || actualVulkanVersion < vulkanVersion) {
            fflush(stdout);
            fprintf(stderr, "Unsupported vulkan version! %x\n", instanceVersionResult);
            exit(1);
        }
    }

    const char* const InstanceLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    const size_t InstanceLayersCount = sizeof(InstanceLayers) / sizeof(InstanceLayers[0]);

    const char* const InstanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    const size_t InstanceExtensionsCount = sizeof(InstanceExtensions) / sizeof(InstanceExtensions[0]);

    VkInstance instance = VK_NULL_HANDLE;
    {
        uint32_t availableInstanceLayerCount = 0;
        VkCheck(vkEnumerateInstanceLayerProperties(&availableInstanceLayerCount, NULL));
        VkLayerProperties availableInstanceLayers[availableInstanceLayerCount];
        VkCheck(vkEnumerateInstanceLayerProperties(&availableInstanceLayerCount, availableInstanceLayers));
        for (size_t i = 0; i < InstanceLayersCount; i++) {
            bool foundLayer = false;
            for (uint32_t j = 0; j < availableInstanceLayerCount; j++) {
                if (strcmp(InstanceLayers[i], availableInstanceLayers[j].layerName) == 0) {
                    foundLayer = true;
                    break;
                }
            }
            if (!foundLayer) {
                fflush(stdout);
                fprintf(stderr, "Unsupported instance layer '%s'!\n", InstanceLayers[i]);
                exit(1);
            }
        }

        uint32_t availableInstanceExtensionCount = 0;
        VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &availableInstanceExtensionCount, NULL));
        VkExtensionProperties availableInstanceExtensions[availableInstanceExtensionCount];
        VkCheck(vkEnumerateInstanceExtensionProperties(NULL, &availableInstanceExtensionCount, availableInstanceExtensions));
        for (size_t i = 0; i < InstanceExtensionsCount; i++) {
            bool foundExtension = false;
            for (uint32_t j = 0; j < availableInstanceExtensionCount; j++) {
                if (strcmp(InstanceExtensions[i], availableInstanceExtensions[j].extensionName) == 0) {
                    foundExtension = true;
                    break;
                }
            }
            if (!foundExtension) {
                fflush(stdout);
                fprintf(stderr, "Unsupported instance extension '%s'!\n", InstanceExtensions[i]);
                exit(1);
            }
        }

        const VkResult instanceCreateResult = vkCreateInstance(
            &(VkInstanceCreateInfo){
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pApplicationInfo =
                    &(VkApplicationInfo){
                        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                        .pApplicationName   = "Vulkan Testing",
                        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                        .pEngineName        = "Vulkan Testing",
                        .engineVersion      = VK_MAKE_VERSION(0, 0, 1),
                        .apiVersion         = vulkanVersion,
                    },
                .enabledLayerCount       = InstanceLayersCount,
                .ppEnabledLayerNames     = InstanceLayers,
                .enabledExtensionCount   = InstanceExtensionsCount,
                .ppEnabledExtensionNames = InstanceExtensions,
            },
            allocator,
            &instance);
        if (instanceCreateResult != VK_SUCCESS || instance == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create a vulkan instance! %x\n", instanceCreateResult);
            exit(1);
        }
    }
    printf("Created the vulkan instance!\n");

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    {
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
            cast(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        assert(vkCreateDebugUtilsMessengerEXT);

        VkResult debugUtilsCreateResult = vkCreateDebugUtilsMessengerEXT(
            instance,
            &(VkDebugUtilsMessengerCreateInfoEXT){
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = &DebugMessengerCallback,
            },
            allocator,
            &debugMessenger);
        if (debugUtilsCreateResult != VK_SUCCESS || debugMessenger == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create a debug messenger! %x\n", debugUtilsCreateResult);
            exit(1);
        }
    }
    printf("Created the debug messenger!\n");

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    {
        VkResult surfaceCreateResult = vkCreateWin32SurfaceKHR(instance,
                                                               &(VkWin32SurfaceCreateInfoKHR){
                                                                   .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                                                                   .hinstance = hinstance,
                                                                   .hwnd      = windowHandle,
                                                               },
                                                               allocator,
                                                               &surface);
        if (surfaceCreateResult != VK_SUCCESS || surface == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create a surface! %x\n", surfaceCreateResult);
            exit(1);
        }
    }
    printf("Created a surface!\n");

    const char* const DeviceLayers[] = {};
    const size_t DeviceLayersCount   = sizeof(DeviceLayers) / sizeof(DeviceLayers[0]);

    const char* const DeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    const size_t DeviceExtensionsCount = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

    VkPhysicalDevice physicalDevice   = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uint32_t presentQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    {
        uint32_t physicalDeviceCount = 0;
        VkCheck(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL));
        VkPhysicalDevice physicalDevices[physicalDeviceCount];
        VkCheck(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));
        for (uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount; physicalDeviceIndex++) {
            VkPhysicalDevice currentPhysicalDevice = physicalDevices[physicalDeviceIndex];

            uint32_t availableDeviceLayerCount = 0;
            VkCheck(vkEnumerateDeviceLayerProperties(currentPhysicalDevice, &availableDeviceLayerCount, NULL));
            VkLayerProperties availableDeviceLayers[availableDeviceLayerCount];
            VkCheck(vkEnumerateDeviceLayerProperties(currentPhysicalDevice, &availableDeviceLayerCount, availableDeviceLayers));
            bool hasAllLayers = true;
            for (size_t i = 0; i < DeviceLayersCount; i++) {
                bool foundLayer = false;
                for (uint32_t j = 0; j < availableDeviceLayerCount; j++) {
                    if (strcmp(DeviceLayers[i], availableDeviceLayers[j].layerName) == 0) {
                        foundLayer = true;
                        break;
                    }
                }
                if (!foundLayer) {
                    hasAllLayers = false;
                    break;
                }
            }
            if (!hasAllLayers)
                continue;

            uint32_t availableDeviceExtensionCount = 0;
            VkCheck(vkEnumerateDeviceExtensionProperties(currentPhysicalDevice, NULL, &availableDeviceExtensionCount, NULL));
            VkExtensionProperties availableDeviceExtensions[availableDeviceExtensionCount];
            VkCheck(vkEnumerateDeviceExtensionProperties(
                currentPhysicalDevice, NULL, &availableDeviceExtensionCount, availableDeviceExtensions));
            bool hasAllExtensions = true;
            for (size_t i = 0; i < DeviceExtensionsCount; i++) {
                bool foundExtension = false;
                for (uint32_t j = 0; j < availableDeviceExtensionCount; j++) {
                    if (strcmp(DeviceExtensions[i], availableDeviceExtensions[j].extensionName) == 0) {
                        foundExtension = true;
                        break;
                    }
                }
                if (!foundExtension) {
                    hasAllExtensions = false;
                    break;
                }
            }
            if (!hasAllExtensions)
                continue;

            uint32_t tempGraphicsQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            uint32_t tempPresentQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;

            uint32_t queueFamilyPropertiesCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(currentPhysicalDevice, &queueFamilyPropertiesCount, NULL);
            VkQueueFamilyProperties queueFamilyProperties[queueFamilyPropertiesCount];
            vkGetPhysicalDeviceQueueFamilyProperties(currentPhysicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties);

            for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
                if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    tempGraphicsQueueFamilyIndex = i;

                    VkBool32 presentSupport = false;
                    VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(currentPhysicalDevice, i, surface, &presentSupport));

                    if (presentSupport && vkGetPhysicalDeviceWin32PresentationSupportKHR(currentPhysicalDevice, i)) {
                        tempPresentQueueFamilyIndex = i;
                        break;
                    }
                }
            }

            if (tempPresentQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED) {
                for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
                    VkBool32 presentSupport = false;
                    VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(currentPhysicalDevice, i, surface, &presentSupport));

                    if (presentSupport && vkGetPhysicalDeviceWin32PresentationSupportKHR(currentPhysicalDevice, i)) {
                        tempPresentQueueFamilyIndex = i;
                        break;
                    }
                }
            }

            if (tempGraphicsQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED || tempPresentQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED)
                continue;

            VkPhysicalDeviceProperties properties = {};
            vkGetPhysicalDeviceProperties(currentPhysicalDevice, &properties);
            if (properties.apiVersion < vulkanVersion)
                continue;

            physicalDevice           = currentPhysicalDevice;
            graphicsQueueFamilyIndex = tempGraphicsQueueFamilyIndex;
            presentQueueFamilyIndex  = tempPresentQueueFamilyIndex;

            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                break;
        }
        if (physicalDevice == VK_NULL_HANDLE || graphicsQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED ||
            presentQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED) {
            fflush(stdout);
            fprintf(stderr, "Failed to find a suitable physical device!\n");
            exit(1);
        }
    }
    {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        printf("Chose physical device '%s'!\n", properties.deviceName);
    }

    VkDevice device = VK_NULL_HANDLE;
    {
        VkResult deviceCreateResult =
            vkCreateDevice(physicalDevice,
                           &(VkDeviceCreateInfo){
                               .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                               .queueCreateInfoCount = graphicsQueueFamilyIndex == presentQueueFamilyIndex ? 1 : 2,
                               .pQueueCreateInfos =
                                   (VkDeviceQueueCreateInfo[2]){
                                       {
                                           .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                           .queueFamilyIndex = graphicsQueueFamilyIndex,
                                           .queueCount       = 1,
                                           .pQueuePriorities = (float[1]){ 1.0f },
                                       },
                                       {
                                           .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                           .queueFamilyIndex = presentQueueFamilyIndex,
                                           .queueCount       = 1,
                                           .pQueuePriorities = (float[1]){ 1.0f },
                                       },
                                   },
                               .enabledLayerCount       = DeviceLayersCount,
                               .ppEnabledLayerNames     = DeviceLayers,
                               .enabledExtensionCount   = DeviceExtensionsCount,
                               .ppEnabledExtensionNames = DeviceExtensions,
                           },
                           allocator,
                           &device);
        if (deviceCreateResult != VK_SUCCESS || device == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create a logical device! %x\n", deviceCreateResult);
            exit(1);
        }
    }
    printf("Created logical device!\n");

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    if (graphicsQueue == VK_NULL_HANDLE) {
        fflush(stdout);
        fprintf(stderr, "Failed to get the graphics queue!\n");
        exit(1);
    }

    VkQueue presentQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
    if (presentQueue == VK_NULL_HANDLE) {
        fflush(stdout);
        fprintf(stderr, "Failed to get the present queue!\n");
        exit(1);
    }

    VkSwapchainKHR swapchain           = VK_NULL_HANDLE;
    VkSurfaceFormatKHR swapchainFormat = {};
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
        VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

        uint32_t presentModeCount = 0;
        VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL));
        VkPresentModeKHR presentModes[presentModeCount];
        VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes));

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (uint32_t i = 0; i < presentModeCount; i++) {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = presentModes[i];
                break;
            }
        }

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (imageCount > surfaceCapabilities.maxImageCount) {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        uint32_t surfaceFormatCount = 0;
        VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, NULL));
        VkSurfaceFormatKHR surfaceFormats[surfaceFormatCount];
        VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats));

        assert(surfaceFormatCount > 0);
        swapchainFormat = surfaceFormats[0];
        if (surfaceFormatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
            swapchainFormat.format     = VK_FORMAT_R8G8B8A8_UNORM;
            swapchainFormat.colorSpace = surfaceFormats[0].colorSpace;
        } else {
            for (uint32_t i = 0; i < surfaceFormatCount; i++) {
                if ((surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                     surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM) &&
                    surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    swapchainFormat = surfaceFormats[i];
                    break;
                }
            }
        }

        VkExtent2D extents = {};
        if (surfaceCapabilities.currentExtent.width != ~0u && surfaceCapabilities.currentExtent.height != ~0u) {
            extents = surfaceCapabilities.currentExtent;
        } else {
            uint32_t width  = cast(uint32_t) WindowWidth;
            uint32_t height = cast(uint32_t) WindowHeight;

            if (width > surfaceCapabilities.maxImageExtent.width) {
                width = surfaceCapabilities.maxImageExtent.width;
            } else if (width < surfaceCapabilities.minImageExtent.width) {
                width = surfaceCapabilities.minImageExtent.width;
            }

            if (height > surfaceCapabilities.maxImageExtent.height) {
                height = surfaceCapabilities.maxImageExtent.height;
            } else if (height < surfaceCapabilities.minImageExtent.height) {
                height = surfaceCapabilities.minImageExtent.height;
            }

            extents = (VkExtent2D){ .width = width, .height = height };
        }

        VkResult swapchainCreateResult = vkCreateSwapchainKHR(
            device,
            &(VkSwapchainCreateInfoKHR){
                .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface          = surface,
                .minImageCount    = imageCount,
                .imageFormat      = swapchainFormat.format,
                .imageColorSpace  = swapchainFormat.colorSpace,
                .imageExtent      = extents,
                .imageArrayLayers = 1,
                .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .imageSharingMode =
                    graphicsQueueFamilyIndex != presentQueueFamilyIndex ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = graphicsQueueFamilyIndex != presentQueueFamilyIndex ? 2 : 1,
                .pQueueFamilyIndices   = (uint32_t[2]){ graphicsQueueFamilyIndex, presentQueueFamilyIndex },
                .preTransform          = surfaceCapabilities.currentTransform,
                .compositeAlpha        = (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
                                             ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
                                         : (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
                                             ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
                                         : (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
                                             ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
                                             : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                .presentMode           = presentMode,
                .clipped               = VK_TRUE,
                .oldSwapchain          = swapchain,
            },
            allocator,
            &swapchain);
        if (swapchainCreateResult != VK_SUCCESS || swapchain == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create swapchain! %x\n", swapchainCreateResult);
            exit(1);
        }
    }
    printf("Created the swapchain!\n");

    uint32_t swapchainImageCount = 0;
    VkCheck(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL));
    VkImage swapchainImages[swapchainImageCount];
    VkCheck(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages));

    VkImageView swapchainImageViews[swapchainImageCount];
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VkResult imageViewCreateResult = vkCreateImageView(device,
                                                           &(VkImageViewCreateInfo){
                                                               .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                                               .image    = swapchainImages[i],
                                                               .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                                               .format   = swapchainFormat.format,
                                                               .subresourceRange =
                                                                   (VkImageSubresourceRange){
                                                                       .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                       .levelCount = 1,
                                                                       .layerCount = 1,
                                                                   },
                                                           },
                                                           allocator,
                                                           &swapchainImageViews[i]);
        if (imageViewCreateResult != VK_SUCCESS || swapchainImageViews[i] == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create swapchain image view %d! %x\n", i, imageViewCreateResult);
            exit(1);
        }
    }

    VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
    {
        VkResult commandPoolCreateResult = vkCreateCommandPool(device,
                                                               &(VkCommandPoolCreateInfo){
                                                                   .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                                                   .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                                                   .queueFamilyIndex = graphicsQueueFamilyIndex,
                                                               },
                                                               allocator,
                                                               &graphicsCommandPool);
        if (commandPoolCreateResult != VK_SUCCESS || graphicsCommandPool == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create graphics command pool! %x\n", commandPoolCreateResult);
            exit(1);
        }
    }

    VkCommandBuffer graphicsCommandBuffer = VK_NULL_HANDLE;
    {
        VkResult commandBufferAllocateResult =
            vkAllocateCommandBuffers(device,
                                     &(VkCommandBufferAllocateInfo){
                                         .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                         .commandPool        = graphicsCommandPool,
                                         .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                         .commandBufferCount = 1,
                                     },
                                     &graphicsCommandBuffer);
        if (commandBufferAllocateResult != VK_SUCCESS || graphicsCommandBuffer == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create graphics command buffer! %x\n", commandBufferAllocateResult);
            exit(1);
        }
    }

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    {
        VkResult semaphoreCreateResult = vkCreateSemaphore(device,
                                                           &(VkSemaphoreCreateInfo){
                                                               .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                                                           },
                                                           allocator,
                                                           &imageAvailableSemaphore);
        if (semaphoreCreateResult != VK_SUCCESS || imageAvailableSemaphore == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create image available semaphore! %x\n", semaphoreCreateResult);
            exit(1);
        }
    }

    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    {
        VkResult semaphoreCreateResult = vkCreateSemaphore(device,
                                                           &(VkSemaphoreCreateInfo){
                                                               .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                                                           },
                                                           allocator,
                                                           &renderFinishedSemaphore);
        if (semaphoreCreateResult != VK_SUCCESS || renderFinishedSemaphore == VK_NULL_HANDLE) {
            fflush(stdout);
            fprintf(stderr, "Failed to create render finished semaphore! %x\n", semaphoreCreateResult);
            exit(1);
        }
    }

    while (Running) {
        {
            MSG message;
            while (PeekMessageA(&message, windowHandle, 0, 0, PM_REMOVE)) {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
        }

        uint32_t imageIndex = 0;
        VkCheck(vkAcquireNextImageKHR(device, swapchain, ~0ull, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex));

        VkCheck(vkResetCommandPool(device, graphicsCommandPool, 0));
        VkCheck(vkBeginCommandBuffer(graphicsCommandBuffer,
                                     &(VkCommandBufferBeginInfo){
                                         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                         .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                     }));

        vkCmdPipelineBarrier(graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0,
                             NULL,
                             0,
                             NULL,
                             1,
                             &(VkImageMemoryBarrier){
                                 .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                 .srcAccessMask       = 0,
                                 .dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                 .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                                 .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                 .image               = swapchainImages[imageIndex],
                                 .subresourceRange =
                                     (VkImageSubresourceRange){
                                         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                         .levelCount = VK_REMAINING_MIP_LEVELS,
                                         .layerCount = VK_REMAINING_ARRAY_LAYERS,
                                     },
                             });

        vkCmdClearColorImage(graphicsCommandBuffer,
                             swapchainImages[imageIndex],
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &(VkClearColorValue){
                                 .float32 = { 1.0f, 0.0f, 0.0f, 1.0f },
                             },
                             1,
                             &(VkImageSubresourceRange){
                                 .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseMipLevel   = 0,
                                 .levelCount     = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount     = 1,
                             });

        vkCmdPipelineBarrier(graphicsCommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0,
                             NULL,
                             0,
                             NULL,
                             1,
                             &(VkImageMemoryBarrier){
                                 .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                 .srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                 .dstAccessMask       = 0,
                                 .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                                 .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                 .image               = swapchainImages[imageIndex],
                                 .subresourceRange =
                                     (VkImageSubresourceRange){
                                         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                         .levelCount = VK_REMAINING_MIP_LEVELS,
                                         .layerCount = VK_REMAINING_ARRAY_LAYERS,
                                     },
                             });

        VkCheck(vkEndCommandBuffer(graphicsCommandBuffer));

        VkCheck(vkQueueSubmit(graphicsQueue,
                              1,
                              &(VkSubmitInfo){
                                  .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                  .waitSemaphoreCount = 1,
                                  .pWaitSemaphores    = &imageAvailableSemaphore,
                                  .pWaitDstStageMask =
                                      &(VkPipelineStageFlags){
                                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      },
                                  .commandBufferCount   = 1,
                                  .pCommandBuffers      = &graphicsCommandBuffer,
                                  .signalSemaphoreCount = 1,
                                  .pSignalSemaphores    = &renderFinishedSemaphore,
                              },
                              VK_NULL_HANDLE));

        VkCheck(vkQueuePresentKHR(presentQueue,
                                  &(VkPresentInfoKHR){
                                      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                      .waitSemaphoreCount = 1,
                                      .pWaitSemaphores    = &renderFinishedSemaphore,
                                      .swapchainCount     = 1,
                                      .pSwapchains        = &swapchain,
                                      .pImageIndices      = &imageIndex,
                                  }));

        VkCheck(vkDeviceWaitIdle(device));
    }

    vkDeviceWaitIdle(device);
    vkDestroySemaphore(device, renderFinishedSemaphore, allocator);
    vkDestroySemaphore(device, imageAvailableSemaphore, allocator);
    vkDestroyCommandPool(device, graphicsCommandPool, allocator);
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        vkDestroyImageView(device, swapchainImageViews[i], allocator);
    }
    vkDestroySwapchainKHR(device, swapchain, allocator);
    vkDestroyDevice(device, allocator);

    vkDestroySurfaceKHR(instance, surface, allocator);
    {
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
            cast(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        assert(vkDestroyDebugUtilsMessengerEXT);
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, allocator);
    }
    vkDestroyInstance(instance, allocator);

    DestroyWindow(windowHandle);
    UnregisterClassA(WindowClassName, hinstance);

    return 0;
}
