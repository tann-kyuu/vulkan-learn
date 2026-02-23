#include "test_vulkan.hpp"
#include <cstdint>
#include <limits>
#include <set>
#include <algorithm>
#include <iostream>
#include <cstring>

// --- Constructor --- //
test::test() 
:
    test({1280, 1080, "vulkan"})
{}

test::test(windowInfo window_info)
:
    w_info(window_info),
    window(nullptr)
{
    initWindow();
    initVulkan();
}


// --- Init --- //
void test::initWindow()
{
    if (glfwInit()==GLFW_FALSE)
    {
        throw std::runtime_error("Failed to init glfw!");
    }
    //-- Tell GLFW to not create an OpenGL context --//
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //-- Disable window resizing --//
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    //-- Create the windowed mode window and its Vulkan context --//
    window = glfwCreateWindow(w_info.width, w_info.height, w_info.title.c_str(), nullptr, nullptr);

    if (window ==nullptr)
    {
        throw std::runtime_error("Failed to create window!");
    }
}

void test::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickupPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
}

// --- Debug Utils Messenger --- //
void test::destoryDebugUtilsMessenger(
    VkInstance c_instance, 
    VkDebugUtilsMessengerEXT c_debugMessenger, 
    VkAllocationCallbacks* c_allocation
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(c_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(c_instance, c_debugMessenger, c_allocation);
    }
}


// 创建Vulkan实例
void test::createInstance()
{
    // check IF there is V layer support (if needed), raise if error
    // 检查验证层支持
    if (enabledValidationLayer && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layer is not avaliable!");
    }
    // supply APPLICATION information to Vulkan
    //向Vulkan提供应用程序信息
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // request Vulkan enxtention required by GLFW
    // 请求GLFW所需的Vulkan扩展
    uint32_t glfwExtensionCount;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> Extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enabledValidationLayer)
    {
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    //create VULKAN instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = Extensions.size();
    createInfo.ppEnabledExtensionNames = Extensions.data();
    if (enabledValidationLayer)
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfoEXT;
        createInfo.enabledLayerCount = LayerNames.size();
        createInfo.ppEnabledLayerNames = LayerNames.data();
        populateDebugMessengerCreateInfo(createInfoEXT);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&createInfoEXT;
    }
    else
    {
        createInfo.enabledLayerCount = 0; // TODO : add validation layer support
        createInfo.ppEnabledLayerNames = nullptr;
    }
    VkResult results;
    if ((results = vkCreateInstance(&createInfo, nullptr, &instance)) != VK_SUCCESS)
    {
        switch (results)
        {
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            std::cout << "Driver unsupported!" << std::endl;
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            std::cout << "VkExtension unsupported!" << std::endl;
            break;
        default:
            std::cout << "Unknown error!" << std::endl;
        }
        throw std::runtime_error("Failed to create instance!");
    }
}

//检查验证层支持
bool test::checkValidationLayerSupport()
{
    // Get avaliable layers
    uint32_t avaliableLayerCount;
    vkEnumerateInstanceLayerProperties(&avaliableLayerCount, nullptr);
    std::vector<VkLayerProperties> avaliableLayerProperties(avaliableLayerCount);
    vkEnumerateInstanceLayerProperties(&avaliableLayerCount, avaliableLayerProperties.data());
    /* Check requested layers with avaliable layers
     * If all requested layers are found, return true
     * else return false
    */
    for (const char* LayerName : LayerNames)
    {
        bool layerFound = false;
        for (const auto& layerProperty : avaliableLayerProperties) 
        {
            if (strcmp(LayerName, layerProperty.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }
    return true;
}

void test::pickupPhysicalDevice() {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to enumerate physical devices!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    // check device suitability, choose
    for (const auto& physicalDevice : devices) {
        if (isDeviceSuitable(physicalDevice)) {
            device = physicalDevice;
            break;
        } 
    }
    if (device == nullptr) {
        throw std::runtime_error("None of devices is suitable!");
    }
}

bool test::isDeviceSuitable(VkPhysicalDevice c_device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(c_device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(c_device, &deviceFeatures);
    std::cout << "Found suitable device: "
              << deviceProperties.deviceType << " | "
              << deviceProperties.deviceID   << " | "
              << deviceProperties.deviceName << " | "
              << deviceProperties.driverVersion << std::endl;
    // using discrete gpu with geometry shader
    q_Family = findQueueFamilyIndex(c_device);
    bool deviceExtensionSupported = checkDeviceExtensionSupported(c_device);
    bool SwapChainAdequate = false;
    if (deviceExtensionSupported) {
        SwapChainDetails detail = querySwapChainSupport(c_device);
        SwapChainAdequate = detail.formats.empty() && detail.modes.empty();
    }
    // check queue family of the device
    // Choose device if all requirements are met
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                                        deviceFeatures.geometryShader &&
                                        q_Family.isComplete() &&
                                        deviceExtensionSupported) {
        std::cout << "Choice device id " << deviceProperties.deviceID << std::endl;
        return true;
    }
    return false;
    // TODO : Add scored device choice fucntion (rateDeviceSuitability) , delete information output.
}

void test::createLogicalDevice() {
    std::set<uint32_t> indices = {q_Family.graphicsQueueFamily.value(), q_Family.presentQueueFamily.value()};
    float queuePriority = 1.f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t index : indices) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    VkPhysicalDeviceFeatures features{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data(); // TODO : set queue create info
    createInfo.pEnabledFeatures = &features;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (vkCreateDevice(device, &createInfo, nullptr, &logicDevice) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }
    vkGetDeviceQueue(logicDevice, q_Family.graphicsQueueFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicDevice, q_Family.presentQueueFamily.value(), 0, &presentQueue);
}

bool test::checkDeviceExtensionSupported(VkPhysicalDevice c_device) 
{
    uint32_t deviceExtensionCount;
    vkEnumerateDeviceExtensionProperties(c_device, nullptr, &deviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> deviceExtensionsP(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(c_device, nullptr, &deviceExtensionCount, deviceExtensionsP.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& Extension : deviceExtensionsP) {
        requiredExtensions.erase(Extension.extensionName);
    }
    return requiredExtensions.empty();
}  

queueFamily test::findQueueFamilyIndex(VkPhysicalDevice c_device) {
    uint32_t queueFamiliesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(c_device, &queueFamiliesCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesCount);        
    vkGetPhysicalDeviceQueueFamilyProperties(c_device, &queueFamiliesCount, queueFamilies.data());

    int index = 0;
    queueFamily foundQueueFamily;
    for (const auto& QF : queueFamilies) {
        if (QF.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            foundQueueFamily.graphicsQueueFamily = index;
        }
        VkBool32 presentSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(c_device, index, surface, &presentSupported);
        if (presentSupported) {
            foundQueueFamily.presentQueueFamily = index;
        }
        if (foundQueueFamily.isComplete()) {
            break;
        }
        ++index;
    }
    return foundQueueFamily;
}

SwapChainDetails test::querySwapChainSupport(VkPhysicalDevice c_device)
{
    SwapChainDetails details{};
    details.cap = GetSurfaceCap(c_device);
    details.formats = GetSurfaceFmt(c_device);
    details.modes = GetSurfacePM(c_device);
    return details;
}

VkSurfaceCapabilitiesKHR test::GetSurfaceCap(VkPhysicalDevice c_device) {
    VkSurfaceCapabilitiesKHR SurCap;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(c_device, surface, &SurCap);
    return SurCap;
}

std::vector<VkSurfaceFormatKHR> test::GetSurfaceFmt(VkPhysicalDevice c_device) {
    std::vector<VkSurfaceFormatKHR> SurFmt{};
    uint32_t FmtCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(c_device, surface, &FmtCount, nullptr);
    if (FmtCount != 0) {
        SurFmt.resize(FmtCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(c_device, surface, &FmtCount, SurFmt.data());
    }
    return SurFmt;
}

std::vector<VkPresentModeKHR> test::GetSurfacePM(VkPhysicalDevice c_device) {
    std::vector<VkPresentModeKHR> PModes{};
    uint32_t PModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(c_device, surface, &PModeCount, nullptr);
    if (PModeCount != 0) {
        PModes.resize(PModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(c_device, surface, &PModeCount, PModes.data());
    }
    return PModes;
}

VkSurfaceFormatKHR test::chooseSurfaceFmt(const SwapChainDetails& details) {
    const std::vector<VkSurfaceFormatKHR>& availableFormats = details.formats;
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    } 

    return availableFormats[0];
}

VkPresentModeKHR test::choosePresentMode(const SwapChainDetails& details) {
    const std::vector<VkPresentModeKHR>& availablePresentModes = details.modes;
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D test::chooseExtent2D(const SwapChainDetails& details) {
    const VkSurfaceCapabilitiesKHR& capabilities = details.cap;
    
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else { // 出现自定义选择Extent时的处理
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

// --- Swap Chain --- //
void test::createSwapChain() {
    SwapChainDetails details = querySwapChainSupport(device);

    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFmt(details);
    VkPresentModeKHR presentMode = choosePresentMode(details);
    VkExtent2D extent = chooseExtent2D(details);
    
    uint32_t imageCount = details.cap.minImageCount + 1;
    if (details.cap.maxImageCount > 0 && imageCount > details.cap.maxImageCount) {
        imageCount = details.cap.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    queueFamily q_F = findQueueFamilyIndex(device);
    uint32_t queueFamilyIndices[] = {q_F.graphicsQueueFamily.value(), q_F.presentQueueFamily.value()};
    if (q_F.graphicsQueueFamily != q_F.presentQueueFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2; //TODO
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } 
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // 窗口Alpha通道处理 ：不透明
    createInfo.preTransform = details.cap.currentTransform;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE; // TODO

    if (vkCreateSwapchainKHR(logicDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }
}

void test::getSwapChainImages() {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(logicDevice, swapChain, &imageCount, nullptr);
    if (imageCount != 0) {
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicDevice, swapChain, &imageCount, swapChainImages.data());
    }
}

void test::createImageView() {
    imageViews.resize(swapChainImages.size());
    for (int index = 0; index < swapChainImages.size(); ++index) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[index];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; //
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY; //
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY; //
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY; // 不进行颜色分量映射
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // 颜色图像
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(logicDevice, &createInfo, nullptr, &imageViews[index]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view!");
        }
    }
}

// --- Debug Callback --- //
VKAPI_ATTR VkBool32 VKAPI_CALL test::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageServerity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* messengerData,
    void* pUserData
    ) {
    std::cerr << "Validation Layer: " << messengerData->pMessage << std::endl;
    if (messageServerity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        return VK_TRUE;
    }
    return VK_FALSE;
}


// --- Main loop --- //


void test::run()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

void test::setupDebugMessenger()
{
    if (!enabledValidationLayer) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (createDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create debug utils messenger!");
    }
}

//调试信使创建信息填充
void test::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessenger)
{
    debugUtilsMessenger = {};
    debugUtilsMessenger.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessenger.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    debugUtilsMessenger.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    debugUtilsMessenger.pfnUserCallback = debugCallback;
    debugUtilsMessenger.pUserData = nullptr; // Optional
    
}

VkResult test::createDebugUtilsMessenger(
    VkInstance c_instance, 
    VkDebugUtilsMessengerCreateInfoEXT* c_createInfo, 
    VkAllocationCallbacks* c_allocation, 
    VkDebugUtilsMessengerEXT* c_debugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(c_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func == nullptr)
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    } else {
        return func(c_instance, c_createInfo, c_allocation, c_debugMessenger);
    }
}

// Surface:
// 1.绑定原生窗口
// 2.协商显示参数
// 2.缓冲区交换
void test::createSurface()
{
    //using GLFW surface
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to Create Surface_KHR!");
    }
}

// --- Cleanup --- //
test::~test()
{
    cleanupAll();
}

void test::cleanupAll()
{
    cleanupVulkan();
    cleanupWindow();
}



void test::cleanupWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void test::cleanupVulkan()
{
    vkDestroySwapchainKHR(logicDevice, swapChain, nullptr);
    vkDestroyDevice(logicDevice, nullptr);
    if (enabledValidationLayer)
    {
        destoryDebugUtilsMessenger(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}