#include <string>
#include <sys/stat.h>
#include <vector>
#include <optional>

#define GLFW_INCLUDE_VULKAN // include Vulkan by include glfw with vulkan
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
}; // enumerate required Device Extensions

#ifdef NDEBUG
    const bool enabledValidationLayer = false;
#else
    const bool enabledValidationLayer = true;
#endif

// Required layers
const std::vector<const char *> LayerNames = {
    "VK_LAYER_KHRONOS_validation"
};

struct windowInfo
{
    const int width;
    const int height;
    const std::string title;
};

struct queueFamily
{
    std::optional<uint32_t> graphicsQueueFamily;
    std::optional<uint32_t> presentQueueFamily;
    bool isComplete() {
        return graphicsQueueFamily.has_value() &&
                presentQueueFamily.has_value();
    }
};

struct SwapChainDetails {
    VkSurfaceCapabilitiesKHR cap;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> modes;
};

class test
{
public:
    test();
    test(windowInfo window_info);
    ~test();
public:
    void run();
private:
    void initWindow();
    void initVulkan();
    bool checkValidationLayerSupport();
    void createInstance();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessenger);
    VkResult createDebugUtilsMessenger(VkInstance c_instance, 
                                          VkDebugUtilsMessengerCreateInfoEXT* c_createInfo, 
                                          VkAllocationCallbacks* c_allocation, 
                                          VkDebugUtilsMessengerEXT* c_debugMessenger);
    void destoryDebugUtilsMessenger(VkInstance c_instance, 
                                       VkDebugUtilsMessengerEXT c_debugMessenger, 
                                       VkAllocationCallbacks* c_allocation);
    void pickupPhysicalDevice();
    void createSurface();
    bool isDeviceSuitable(VkPhysicalDevice c_device);
    queueFamily findQueueFamilyIndex(VkPhysicalDevice c_device);
    void createLogicalDevice();
    bool checkDeviceExtensionSupported(VkPhysicalDevice c_device);
    SwapChainDetails querySwapChainSupport(VkPhysicalDevice c_device);
    VkSurfaceCapabilitiesKHR GetSurfaceCap(VkPhysicalDevice c_device);
    std::vector<VkSurfaceFormatKHR> GetSurfaceFmt(VkPhysicalDevice c_device);
    std::vector<VkPresentModeKHR> GetSurfacePM(VkPhysicalDevice c_device); 
    VkSurfaceFormatKHR chooseSurfaceFmt(const SwapChainDetails& details);
    VkPresentModeKHR choosePresentMode(const SwapChainDetails& details);
    VkExtent2D chooseExtent2D(const SwapChainDetails& details);
    void createSwapChain();
    void getSwapChainImages();
    void createImageViews();
    void createRenderPass();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipeline();
private:
    static std::vector<char> readFile(const std::string& filename);
private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageServerity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* messengerData,
        void* pUserData
    );

private:
    void cleanupWindow();
    void cleanupAll();
    void cleanupVulkan();
private:
    windowInfo w_info;
    GLFWwindow *window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice device;
    VkDevice logicDevice;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> imageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
private:
    queueFamily q_Family;
};