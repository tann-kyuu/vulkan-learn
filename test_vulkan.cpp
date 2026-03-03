#include "test_vulkan.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <fstream>
#include <stdexcept>

std::vector<char> test::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    // ios::ate 从文件末尾开始读取，通过获取末尾指针确定文件与缓冲区大小。
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

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
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
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
        SwapChainAdequate = !detail.formats.empty() && !detail.modes.empty();
    }
    // check queue family of the device
    // Choose device if all requirements are met
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                                        deviceFeatures.geometryShader &&
                                        q_Family.isComplete() &&
                                        deviceExtensionSupported &&
                                        SwapChainAdequate) {
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

    getSwapChainImages();

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void test::getSwapChainImages() {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(logicDevice, swapChain, &imageCount, nullptr);
    if (imageCount != 0) {
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicDevice, swapChain, &imageCount, swapChainImages.data());
    }
}

void test::createImageViews() {
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

void test::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(logicDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

VkShaderModule test::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(logicDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    return shaderModule;
}

void test::createGraphicsPipeline() {
    auto vertShaderCode = readFile("shader.vert.spv");
    auto fragShaderCode = readFile("shader.frag.spv");
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // 着色器阶段创建
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};  
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // **固定功能状态**

    // 动态状态
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 顶点输入
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // optional

    // 输入汇编
    /* `VkPipelineInputAssemblyStateCreateInfo` 结构描述了两件事：
     * 将从顶点绘制的几何图形类型以及是否应启用基元重启。前者在 `topology` 成员中指定，并且可以具有如下值：
     * `VK_PRIMITIVE_TOPOLOGY_POINT_LIST`：来自顶点的点
     * `VK_PRIMITIVE_TOPOLOGY_LINE_LIST`：每 2 个顶点之间的直线，不复用
     * `VK_PRIMITIVE_TOPOLOGY_LINE_STRIP`：每条线的结束顶点用作下一条线的起始顶点
     * `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`：每 3 个顶点组成的三角形，不复用
     * `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP`：每个三角形的第二个和第三个顶点用作下一个三角形的前两个顶点
     * 通常，顶点按顺序从顶点缓冲区按索引加载，但是使用 *元素缓冲区*，您可以自己指定要使用的索引。
     * 这允许您执行诸如复用顶点之类的优化。如果将 `primitiveRestartEnable` 成员设置为 `VK_TRUE`，
     * 则可以使用 `0xFFFF` 或 `0xFFFFFFFF` 的特殊索引来分解 `_STRIP` 拓扑模式中的线和三角形。
     * 我们打算在本教程中绘制三角形，因此我们将坚持使用以下结构的以下数据：
     */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪矩形  (静态状态)
    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // 光栅化器
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; 
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.f; // > 1.0 需物理设备支持wideLines特性
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE; // 深度值偏移
    rasterizer.depthBiasConstantFactor = 0.f;
    rasterizer.depthBiasSlopeFactor = 0.f;
    rasterizer.depthBiasClamp = 0.f;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.minSampleShading = 1.f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToOneEnable = VK_FALSE;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    
    // 深度和模板测试 (如果需要)
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    
    // 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
                                        | VK_COLOR_COMPONENT_G_BIT 
                                        | VK_COLOR_COMPONENT_B_BIT 
                                        | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.f;
    colorBlending.blendConstants[1] = 0.f;
    colorBlending.blendConstants[2] = 0.f;
    colorBlending.blendConstants[3] = 0.f;
    
    // 管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(logicDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(logicDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipelines!");
    }

    vkDestroyShaderModule(logicDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(logicDevice, fragShaderModule, nullptr);
}

void test::createFramebuffers() {
    swapChainFramebuffers.resize(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); ++i) {
        VkImageView attachments[] = {
            imageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(logicDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void test::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = q_Family.graphicsQueueFamily.value();

    if (vkCreateCommandPool(logicDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void test::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(logicDevice, &allocInfo, &commandBuffer)) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void test::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.f, 0.f, 0.f, 1.f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,graphicsPipeline);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}

void test::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateSemaphore(logicDevice, &semaphoreInfo, nullptr, &imageAvaliableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(logicDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(logicDevice, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores!");
    }
}

void test::drawFrame() {
    vkWaitForFences(logicDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(logicDevice, 1, &inFlightFence);
    uint32_t imageIndex;
    vkAcquireNextImageKHR(logicDevice, swapChain, UINT64_MAX, imageAvaliableSemaphore, VK_NULL_HANDLE, &imageIndex);
    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] ={imageAvaliableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(presentQueue, &presentInfo);
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

void test::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(logicDevice);
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

// 调试信使创建信息填充
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


void test::createSurface()
{
    //using GLFW surface
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to Create Surface_KHR!");
    }
}

// --- Cleanup --- //
void test::cleanupWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void test::cleanupVulkan()
{
    vkDestroySemaphore(logicDevice, imageAvaliableSemaphore, nullptr);
    vkDestroySemaphore(logicDevice, renderFinishedSemaphore, nullptr);
    vkDestroyFence(logicDevice, inFlightFence, nullptr);

    vkDestroyCommandPool(logicDevice, commandPool, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(logicDevice, framebuffer, nullptr);
    }

    vkDestroyPipeline(logicDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(logicDevice, pipelineLayout, nullptr);

    vkDestroyRenderPass(logicDevice, renderPass, nullptr);

    for (auto imageView : imageViews) {
        vkDestroyImageView(logicDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(logicDevice, swapChain, nullptr);
    vkDestroyDevice(logicDevice, nullptr);

    if (enabledValidationLayer)
    {
        destoryDebugUtilsMessenger(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void test::cleanupAll()
{
    cleanupVulkan();
    cleanupWindow();
}

test::~test()
{
    cleanupAll();
}