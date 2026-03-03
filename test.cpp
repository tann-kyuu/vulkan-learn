#include "test_vulkan.hpp"

const int width = 1280;
const int height = 1080;
const std::string title = "Triangle";


int main() {
    windowInfo info{width, height, title};
    test t01(info);
    t01.mainLoop();
    return 0;
}