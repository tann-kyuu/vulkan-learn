# Vulkan Learn

这是一个用于学习 Vulkan 初始化流程的起步练手项目，采用类封装Vulkan API，已完成的进度：

- 使用 GLFW 创建窗口与 Vulkan Surface
- 创建 Vulkan Instance、Debug Messenger
- 选择物理设备并创建逻辑设备
- 创建 SwapChain 与 ImageView

当前代码可成功打开窗口并完成基础 Vulkan 初始化，刚刚进入完整渲染管线阶段。

## 项目结构

```text
.
├─ CMakeLists.txt
├─ test.cpp                 # 程序入口
├─ test_vulkan.hpp          # Vulkan 学习类声明
├─ test_vulkan.cpp          # Vulkan 初始化逻辑实现
├─ Shader/
│  ├─ vertexShader.vert
│  └─ fragmentShader.frag
└─ glfw/
	├─ include/
	└─ lib/
```


## 后续学习方向（TODO）

- [x] 创建窗口
- [x] 实例初始化Vulkan库
- [x] 开启验证层
- [x] 获取物理设备
- [x] 队列族
- [x] 逻辑设备与队列
- [x] 窗口表面Surface
- [x] 图像的队列 *交换链*
- [x] 双缓冲与三缓冲
- [x] 解析图像 *图像视图*
- [x] 图像管线基础
- [x] 创建图形管线 *可编程阶段*
- [x] 创建图形管线 *固定功能状态*
- [x] 创建图形管线 *渲染通道*
- [x] 保存管线配置
- [ ] 创建帧缓冲区
- [ ] 子通道同步Dependency
- [ ] 命令池与命令缓冲区
- [ ] **渲染三角形**
- [ ] 优化渲染帧
- [ ] 调整窗口大小
- [ ] 描述顶点数据
- [ ] 顶点缓冲区
- [ ] 更快的顶点缓冲
- [ ] 索引缓冲区
- [ ] 让矩形转起来
- [ ] 读取纹理图像
- [ ] 纹理映射
- [ ] 深度测试
- [ ] 摄像机移动
- [ ] 加载Mesh
- [ ] 使用MipMap
- [ ] MSAA抗锯齿

## 教程 / 参考文档
[[系列一]Vulkan零基础入门教程](https://www.bilibili.com/video/BV1HHg7ztEwc) - Bilibili - *一个DengYay*

[Vulkan Tutorial](https://vulkan-tutorial.com/) - Vulkan Tutorial 英文原文

[VulkanTutorialCN](https://github.com/fangcun010/VulkanTutorialCN) - Vulkan Tutotial 中文版

[How To Learn Vulkan](https://www.jeremyong.com/c++/vulkan/graphics/rendering/2018/03/26/how-to-learn-vulkan/) - 官方指引

[Vulkan Documentation](https://docs.vulkan.net.cn/tutorial/latest/00_Introduction.html) - 官方文档



## 环境要求

- Windows 10/11
- CMake 3.30+
- 支持 C++20 的编译器（当前配置为 MinGW `g++`）
- Vulkan SDK（需包含验证层）
- 本地 GLFW 头文件与库文件

> 本项目通过本地目录引入 GLFW：
> - 头文件：`glfw/include/GLFW/...`
> - 库文件：`glfw/lib-*/...`

> 需自行安装 Vulkan SDK 

## 快速开始（Windows）

### 1. 准备 GLFW

将 GLFW 的文件放入以下目录：

- `glfw/include/GLFW/glfw3.h`
- `glfw/include/GLFW/glfw3native.h`
- `glfw/lib-mingw-w64/` 下放置对应架构的 `glfw3` 库

### 2. 配置与构建

在项目根目录执行：

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

如果你使用 Ninja：

```powershell
cmake -S . -B build -G "Ninja"
cmake --build build
```

### 3. 运行

```powershell
.\build\vulkan_test.exe
```

