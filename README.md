# Vulkan C++ Graphics Engine (Zero Engine)
A simple graphics/game engine built with C++ and Vulkan API, featuring a custom Render Graph, bindless descriptor abstraction, and Slang shaders. 

***

![AutumnHouse](./Resources/AutumnHouse1.png)
*Scene 1: Autumn house by Zhenya, with lighting, shadow maps and skybox in the background.*

![Sponza](./Resources/Sponza.png)
*Scene 2: Sponza model with directional lighting and shadow maps.*

***

## Features & Architectures

* **Platform & API Abstraction:**
  * **Render Hardware Interface (RHI):** Decoupled renderer API layer for clean engine-to-GPU abstraction.
  * **Application / Sandbox Separation:** Separates runtime engine loops from client application via event and layer systems.
* **Modern Vulkan Pipeline:**
  * **Bindless Descriptors:** Large descriptor indexing to minimize CPU-side bind overhead and better organizes descriptor set usage.
  * **Render Graph System:** Automated resource lifetime management, frame pass dependency tracking, and Vulkan barrier execution.
  * **Slang Shaders**
* **Scene & Visibility Management:**
  * **Resource Manager:** Opaque handles management system.
  * **Model Loading:** 3D asset import using `fastgltf`.
  * **Frustum Culling:** CPU-side view-frustum culling to optimize draw calls before submission.
  * **Perspective Camera & Input Systems**

***

## Tech Stack & Dependencies

* **Language & Build System:** C++20, CMake (3.20+)
* **Graphics API:** Vulkan (vulkan_raii)
* **Shading Language:** Slang
* **Libraries:** GLFW, GLM, Dear ImGui, fastgltf, spdlog, KTX (Khronos Texture), stb_image

***

## Getting Started

> ***Note on Platform Support:**  
> This project is currently target-developed and tested specifically for **Windows 10/11 (64-bit)** using **Visual Studio 2022 (MSVC)**. Cross-platform support is planned for future iterations.

<ins>**1. Prerequisites:**</ins>

1. **IDE / Tool-chain:** Visual Studio 2022 (with *Desktop development with C++* workload installed).
2. **CMake:** Version 3.20 or higher.
3. **Vulkan SDK:** Download and install the latest [LunarG Vulkan SDK](https://vulkan.lunarg.com/). Ensure `VULKAN_SDK` environment variable is set.

<ins>**2. Downloading the Repository:**</ins>

Start by cloning the repository with `git clone --recursive https://github.com/Zero-Ya/zero-engine.git`.

If you cloned the repository non-recursively previously, use `git submodule update --init --recursive` to clone the necessary submodules.

<ins>**3. Building and Running:**</ins>

**Building via Visual Studio 2022**
1. Open Visual Studio 2022.
2. Select *File > Open > Folder...* and select the cloned `zero-engine` directory.
3. Visual Studio will automatically detect CMakeLists.txt and configure the project.
> ***Note on Cache and CMake Configure:** Try to only configure it once on Visual Studio startup. If you get errors on configure, close the Visual Studio, go to the root directory and delete the hidden `.vs` folder and the `build` folder and reopen Visual Studio again.
4. Select `Sandbox.exe` from the target dropdown menu and hit F5 (or Ctrl+F5) to build and run.

**Building via Visual Studio Developer Command Prompt**
1. Generate project files: Double-click the [GenerateProject.bat](https://github.com/Zero-Ya/zero-engine/blob/master/GenerateProject.bat) script in the root directory, **OR** run the following in the terminal (of the root directory): `cmake --preset debug`.
2. Compile: Run `cmake --build --preset debug` in the terminal (of the root directory).

If changes are made, or if you want to regenerate project files, rerun the [GenerateProject.bat](https://github.com/Zero-Ya/zero-engine/blob/master/GenerateProject.bat) script in the root directory.

The executable file `Sandbox.exe` can be found in `/build/debug/bin/Debug` (or `/build/release/bin/Release` for release configuration).

***

## Roadmap

Plans for the Project:
1. PBR material and lighting.
2. Cascaded Shadow Maps.
3. Scene management