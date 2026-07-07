#include "ImGuiVulkanUtil.h"

namespace ZEngine {

    static std::vector<char> readFile(const std::string& filename);

    ImGuiVulkanUtil::ImGuiVulkanUtil(VulkanContext* ctx, VulkanSwapchain* swapchain, VulkanCommandManager* command, ResourceManager* resource, VulkanSyncManager* sync)
        : vk_Ctx(ctx), vk_Swapchain(swapchain), vk_CommandManager(command), resourceManager(resource), vk_SyncManager(sync)
    {
        renderingInfo.colorAttachmentCount = 1;
        vk::Format colorFormat = vk_Swapchain->getFormat().format;
        renderingInfo.pColorAttachmentFormats = &colorFormat;
    }

    ImGuiVulkanUtil::~ImGuiVulkanUtil() {
        if (*vk_Ctx->getDevice()) {
            vk_Ctx->getDevice().waitIdle();
        }
    }

    void ImGuiVulkanUtil::init(float width, float height) {
        // Initialize ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Configure ImGui
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable docking
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable multi-viewport

        // Set display size
        io.DisplaySize = ImVec2(width, height);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

        // Set up style
        vulkanStyle = ImGui::GetStyle();
        vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

        // Apply default style
        setStyle(0);
    }

    void ImGuiVulkanUtil::setStyle(uint32_t index) {
        ImGuiStyle& style = ImGui::GetStyle();

        switch (index) {
        case 0:
            // Custom Vulkan style
            style = vulkanStyle;
            break;
        case 1:
            // Classic style
            ImGui::StyleColorsClassic();
            break;
        case 2:
            // Dark style
            ImGui::StyleColorsDark();
            break;
        case 3:
            // Light style
            ImGui::StyleColorsLight();
            break;
        }
    }

    void ImGuiVulkanUtil::initResources() {
        // Extract font atlas data from ImGui's internal font system
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* fontData;
        int texWidth, texHeight;
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

        // Calculate total memory requirements for GPU transfer
        vk::DeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

        // Define image dimensions and create extent structure
        vk::Extent3D fontExtent{
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight),
            1
        };

        // Buffers
        uint32_t frames = MAX_FRAMES_IN_FLIGHT;
        vertexBuffers.clear();
        vertexBuffers.reserve(frames);
        vertexBufferMemories.clear();
        vertexBufferMemories.reserve(frames);
        indexBuffers.clear();
        indexBuffers.reserve(frames);
        indexBufferMemories.clear();
        indexBufferMemories.reserve(frames);
        for (uint32_t i = 0; i < frames; ++i) {
            vertexBuffers.emplace_back(nullptr);
            vertexBufferMemories.emplace_back(nullptr);
            indexBuffers.emplace_back(nullptr);
            indexBufferMemories.emplace_back(nullptr);
        }
        vertexCounts.assign(frames, 0);
        indexCounts.assign(frames, 0);

        // Font texture
        resourceManager->createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, fontImage, fontImageMemory);
        fontImageView = resourceManager->createImageView(fontImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);

        vk::raii::Buffer       stagingBuffer({});
        vk::raii::DeviceMemory stagingBufferMemory({});
        resourceManager->createBuffer(uploadSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

        void* data = stagingBufferMemory.mapMemory(0, uploadSize);
        memcpy(data, fontData, uploadSize);
        stagingBufferMemory.unmapMemory();

        // Transition image properly
        resourceManager->transitionImageLayout(fontImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        resourceManager->copyBufferToImage(stagingBuffer, fontImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        resourceManager->transitionImageLayout(fontImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        // Configure texture sampling parameters for optimal text rendering
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;

        fontSampler = vk_Ctx->getDevice().createSampler(samplerInfo);

        // Create descriptor pool for shader resource binding
        vk::DescriptorPoolSize poolSizes[]{ { vk::DescriptorType::eCombinedImageSampler, 50 } };

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = 50;
        poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
        poolInfo.pPoolSizes = poolSizes;

        descriptorPool = vk_Ctx->getDevice().createDescriptorPool(poolInfo);

        // Create descriptor set layout defining shader resource interface
        vk::DescriptorSetLayoutBinding binding{};
        binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        binding.binding = 0;

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        descriptorSetLayout = vk_Ctx->getDevice().createDescriptorSetLayout(layoutInfo);

        // Allocate descriptor set from pool using the defined layout
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = *descriptorPool;
        allocInfo.descriptorSetCount = 1;
        vk::DescriptorSetLayout layouts[] = { *descriptorSetLayout };
        allocInfo.pSetLayouts = layouts;

        descriptorSet = std::move(vk_Ctx->getDevice().allocateDescriptorSets(allocInfo).front());

        // Update descriptor set with actual font texture and sampler resources
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = fontImageView;
        imageInfo.sampler = *fontSampler;

        vk::WriteDescriptorSet writeSet{};
        writeSet.dstSet = *descriptorSet;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeSet.pImageInfo = &imageInfo;
        writeSet.dstBinding = 0;

        vk_Ctx->getDevice().updateDescriptorSets(writeSet, nullptr);

        // Create pipeline layout
        vk::PushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstBlock);

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setLayoutCount = 1;
        vk::DescriptorSetLayout setLayouts[] = { *descriptorSetLayout };
        pipelineLayoutInfo.pSetLayouts = setLayouts;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        pipelineLayout = vk_Ctx->getDevice().createPipelineLayout(pipelineLayoutInfo);

        // Create the graphics pipeline with dynamic rendering
        createPipeline();
    }

    void ImGuiVulkanUtil::createPipeline()
    {
        vk::raii::ShaderModule shaderModule = createShaderModule(readFile(ASSETS_DIR"/shaders/imgui.spv"));

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain" };
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // sizeof(ImDrawVert)
        vk::VertexInputBindingDescription                  bindingDescription{ 0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex};
        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)),
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)),
                vk::VertexInputAttributeDescription(2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col))
        };

        vk::PipelineVertexInputStateCreateInfo   vertexInputInfo{ .vertexBindingDescriptionCount = 1,
                                                                 .pVertexBindingDescriptions = &bindingDescription,
                                                                 .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
                                                                 .pVertexAttributeDescriptions = attributeDescriptions.data() };

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList };
        vk::PipelineViewportStateCreateInfo      viewportState{ .viewportCount = 1, .scissorCount = 1 };

        vk::PipelineRasterizationStateCreateInfo rasterizer{ .depthClampEnable = vk::False,
                                                            .rasterizerDiscardEnable = vk::False,
                                                            .polygonMode = vk::PolygonMode::eFill,
                                                            .cullMode = vk::CullModeFlagBits::eNone,
                                                            .frontFace = vk::FrontFace::eCounterClockwise,
                                                            .depthBiasEnable = vk::False,
                                                            .lineWidth = 1.0f };

        vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False };

        vk::PipelineDepthStencilStateCreateInfo depthStencil{
            .depthTestEnable = vk::False,
            .depthWriteEnable = vk::False,
            .depthCompareOp = vk::CompareOp::eLessOrEqual,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable = vk::False };

        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

        vk::PipelineColorBlendStateCreateInfo colorBlending{
            .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment };

        std::vector<vk::DynamicState>      dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

        vk::Format depthFormat = resourceManager->findDepthFormat();

        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
            {.pNext = &renderingInfo,
             .stageCount = 2,
             .pStages = shaderStages,
             .pVertexInputState = &vertexInputInfo,
             .pInputAssemblyState = &inputAssembly,
             .pViewportState = &viewportState,
             .pRasterizationState = &rasterizer,
             .pMultisampleState = &multisampling,
             .pDepthStencilState = &depthStencil,
             .pColorBlendState = &colorBlending,
             .pDynamicState = &dynamicState,
             .layout = pipelineLayout,
             .renderPass = nullptr },
            {.colorAttachmentCount = 1, .pColorAttachmentFormats = &vk_Swapchain->getFormat().format, .depthAttachmentFormat = depthFormat} };

        pipeline = vk::raii::Pipeline(vk_Ctx->getDevice(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());

    }

    [[nodiscard]] vk::raii::ShaderModule ImGuiVulkanUtil::createShaderModule(const std::vector<char>& code) const
    {
        vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
        vk::raii::ShaderModule     shaderModule{ vk_Ctx->getDevice(), createInfo };

        return shaderModule;
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }
        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        file.close();
        return buffer;
    }

    bool ImGuiVulkanUtil::newFrame() {
        ImGui::NewFrame();

        // Create your UI elements here
        // For example:
        ImGui::Begin("Vulkan ImGui Demo");
        ImGui::Text("Hello, Vulkan!");
        if (ImGui::Button("Click me!")) {
            std::cout << "Button was clicked!" << std::endl;
        }
        ImGui::End();

        // Show the demo window
        ImGui::ShowDemoWindow();

        // End the frame
        ImGui::EndFrame();

        // Render to generate draw data
        ImGui::Render();

        return false;
    }

    void ImGuiVulkanUtil::updateBuffers(uint32_t frameIndex) {
        ImDrawData* drawData = ImGui::GetDrawData();
        if (!drawData || drawData->CmdListsCount == 0) {
            return;
        }

        // Calculate required buffer sizes
        vk::DeviceSize vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
        vk::DeviceSize indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

        // Resize buffers if needed for this frame
        if (frameIndex >= vertexCounts.size())
            return;

        if (static_cast<uint32_t>(drawData->TotalVtxCount) > vertexCounts[frameIndex]) {
            // Clean up old buffer
            vertexBuffers[frameIndex] = vk::raii::Buffer(nullptr);
            vertexBufferMemories[frameIndex] = vk::raii::DeviceMemory(nullptr);

            // Create new vertex buffer
            resourceManager->createBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexBuffers[frameIndex], vertexBufferMemories[frameIndex]);
            vertexCounts[frameIndex] = drawData->TotalVtxCount;
        }

        if (static_cast<uint32_t>(drawData->TotalIdxCount) > indexCounts[frameIndex]) {
            // Clean up old buffer
            indexBuffers[frameIndex] = vk::raii::Buffer(nullptr);
            indexBufferMemories[frameIndex] = vk::raii::DeviceMemory(nullptr);

            // Create new index buffer
            resourceManager->createBuffer(indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexBuffers[frameIndex], indexBufferMemories[frameIndex]);
            indexCounts[frameIndex] = drawData->TotalIdxCount;
        }

        // Upload data to buffers for this frame (only if we have data to upload)
        if (drawData->TotalVtxCount > 0 && drawData->TotalIdxCount > 0) {
            void* vtxMappedMemory = vertexBufferMemories[frameIndex].mapMemory(0, vertexBufferSize);
            void* idxMappedMemory = indexBufferMemories[frameIndex].mapMemory(0, indexBufferSize);

            ImDrawVert* vtxDst = static_cast<ImDrawVert*>(vtxMappedMemory);
            ImDrawIdx* idxDst = static_cast<ImDrawIdx*>(idxMappedMemory);

            for (int n = 0; n < drawData->CmdListsCount; n++) {
                const ImDrawList* cmdList = drawData->CmdLists[n];
                memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                vtxDst += cmdList->VtxBuffer.Size;
                idxDst += cmdList->IdxBuffer.Size;
            }

            vertexBufferMemories[frameIndex].unmapMemory();
            indexBufferMemories[frameIndex].unmapMemory();
        }

    }

    void ImGuiVulkanUtil::drawFrame(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex, uint32_t frameIndex) {
        ImGui::Render();

        updateBuffers(frameIndex);

        ImDrawData* drawData = ImGui::GetDrawData();
        if (!drawData || drawData->CmdListsCount == 0) {
            return;
        }

        // Bind the pipeline used for ImGui
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

        // Configure viewport for UI pixel coordinates
        vk::Viewport viewport{};
        viewport.width = drawData->DisplaySize.x;
        viewport.height = drawData->DisplaySize.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        commandBuffer.setViewport(0, viewport);

        // Push constants
        pushConstBlock.scale[0] = 2.0f / ImGui::GetIO().DisplaySize.x;
        pushConstBlock.scale[1] = 2.0f / ImGui::GetIO().DisplaySize.y;
        pushConstBlock.translate[0] = -1.0f;
        pushConstBlock.translate[1] = -1.0f;

        commandBuffer.pushConstants<PushConstBlock>(*pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, pushConstBlock);

        // Bind vertex and index buffers for this frame
        commandBuffer.bindVertexBuffers(0, *vertexBuffers[frameIndex], vk::DeviceSize{ 0 });
        commandBuffer.bindIndexBuffer(*indexBuffers[frameIndex], 0, vk::IndexType::eUint16);

        int vertexOffset = 0;
        int indexOffset = 0;

        for (int i = 0; i < drawData->CmdListsCount; i++) {
            const ImDrawList* cmdList = drawData->CmdLists[i];

            for (int j = 0; j < cmdList->CmdBuffer.Size; j++) {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[j];

                // Set scissor rectangle
                vk::Rect2D scissor;
                scissor.offset.x = std::max(static_cast<int32_t>(pcmd->ClipRect.x), 0);
                scissor.offset.y = std::max(static_cast<int32_t>(pcmd->ClipRect.y), 0);
                scissor.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissor.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
                commandBuffer.setScissor(0, { scissor });

                // Bind descriptor set (font texture)
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, { *descriptorSet }, {});

                // Draw
                commandBuffer.drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }

            vertexOffset += cmdList->VtxBuffer.Size;
        }
    }

    //ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int keycode, int scancode);
    //void ImGuiVulkanUtil::handleKey(int key, int scancode, int action, int mods) {
    //    ImGuiIO& io = ImGui::GetIO();

    //    // 1. Convert the GLFW/Platform key to an ImGuiKey
    //    // Note: If you are using GLFW, use ImGui_ImplGlfw_KeyToImGuiKey(key) 
    //    // or a simple helper function to map them.
    //    ImGuiKey imgui_key = ImGui_ImplGlfw_KeyToImGuiKey(key, scancode);

    //    // 2. Submit the key event
    //    // AddKeyEvent handles the KeysDown array and all Modifiers (Ctrl, Shift, etc.) automatically
    //    if (imgui_key != ImGuiKey_None) {
    //        io.AddKeyEvent(imgui_key, (action != 0)); // true for Press/Repeat, false for Release
    //    }
    //}

    //ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int keycode, int scancode)
    //{
    //    IM_UNUSED(scancode);
    //    switch (keycode)
    //    {
    //    case GLFW_KEY_TAB: return ImGuiKey_Tab;
    //    case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
    //    case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
    //    case GLFW_KEY_UP: return ImGuiKey_UpArrow;
    //    case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
    //    case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
    //    case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
    //    case GLFW_KEY_HOME: return ImGuiKey_Home;
    //    case GLFW_KEY_END: return ImGuiKey_End;
    //    case GLFW_KEY_INSERT: return ImGuiKey_Insert;
    //    case GLFW_KEY_DELETE: return ImGuiKey_Delete;
    //    case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
    //    case GLFW_KEY_SPACE: return ImGuiKey_Space;
    //    case GLFW_KEY_ENTER: return ImGuiKey_Enter;
    //    case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
    //    case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
    //    case GLFW_KEY_COMMA: return ImGuiKey_Comma;
    //    case GLFW_KEY_MINUS: return ImGuiKey_Minus;
    //    case GLFW_KEY_PERIOD: return ImGuiKey_Period;
    //    case GLFW_KEY_SLASH: return ImGuiKey_Slash;
    //    case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
    //    case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
    //    case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
    //    case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
    //    case GLFW_KEY_WORLD_1: return ImGuiKey_Oem102;
    //    case GLFW_KEY_WORLD_2: return ImGuiKey_Oem102;
    //    case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
    //    case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
    //    case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
    //    case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
    //    case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
    //    case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
    //    case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
    //    case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
    //    case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
    //    case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
    //    case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
    //    case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
    //    case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
    //    case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
    //    case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
    //    case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
    //    case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
    //    case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
    //    case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
    //    case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
    //    case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
    //    case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
    //    case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
    //    case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
    //    case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
    //    case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
    //    case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
    //    case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
    //    case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
    //    case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
    //    case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
    //    case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
    //    case GLFW_KEY_MENU: return ImGuiKey_Menu;
    //    case GLFW_KEY_0: return ImGuiKey_0;
    //    case GLFW_KEY_1: return ImGuiKey_1;
    //    case GLFW_KEY_2: return ImGuiKey_2;
    //    case GLFW_KEY_3: return ImGuiKey_3;
    //    case GLFW_KEY_4: return ImGuiKey_4;
    //    case GLFW_KEY_5: return ImGuiKey_5;
    //    case GLFW_KEY_6: return ImGuiKey_6;
    //    case GLFW_KEY_7: return ImGuiKey_7;
    //    case GLFW_KEY_8: return ImGuiKey_8;
    //    case GLFW_KEY_9: return ImGuiKey_9;
    //    case GLFW_KEY_A: return ImGuiKey_A;
    //    case GLFW_KEY_B: return ImGuiKey_B;
    //    case GLFW_KEY_C: return ImGuiKey_C;
    //    case GLFW_KEY_D: return ImGuiKey_D;
    //    case GLFW_KEY_E: return ImGuiKey_E;
    //    case GLFW_KEY_F: return ImGuiKey_F;
    //    case GLFW_KEY_G: return ImGuiKey_G;
    //    case GLFW_KEY_H: return ImGuiKey_H;
    //    case GLFW_KEY_I: return ImGuiKey_I;
    //    case GLFW_KEY_J: return ImGuiKey_J;
    //    case GLFW_KEY_K: return ImGuiKey_K;
    //    case GLFW_KEY_L: return ImGuiKey_L;
    //    case GLFW_KEY_M: return ImGuiKey_M;
    //    case GLFW_KEY_N: return ImGuiKey_N;
    //    case GLFW_KEY_O: return ImGuiKey_O;
    //    case GLFW_KEY_P: return ImGuiKey_P;
    //    case GLFW_KEY_Q: return ImGuiKey_Q;
    //    case GLFW_KEY_R: return ImGuiKey_R;
    //    case GLFW_KEY_S: return ImGuiKey_S;
    //    case GLFW_KEY_T: return ImGuiKey_T;
    //    case GLFW_KEY_U: return ImGuiKey_U;
    //    case GLFW_KEY_V: return ImGuiKey_V;
    //    case GLFW_KEY_W: return ImGuiKey_W;
    //    case GLFW_KEY_X: return ImGuiKey_X;
    //    case GLFW_KEY_Y: return ImGuiKey_Y;
    //    case GLFW_KEY_Z: return ImGuiKey_Z;
    //    case GLFW_KEY_F1: return ImGuiKey_F1;
    //    case GLFW_KEY_F2: return ImGuiKey_F2;
    //    case GLFW_KEY_F3: return ImGuiKey_F3;
    //    case GLFW_KEY_F4: return ImGuiKey_F4;
    //    case GLFW_KEY_F5: return ImGuiKey_F5;
    //    case GLFW_KEY_F6: return ImGuiKey_F6;
    //    case GLFW_KEY_F7: return ImGuiKey_F7;
    //    case GLFW_KEY_F8: return ImGuiKey_F8;
    //    case GLFW_KEY_F9: return ImGuiKey_F9;
    //    case GLFW_KEY_F10: return ImGuiKey_F10;
    //    case GLFW_KEY_F11: return ImGuiKey_F11;
    //    case GLFW_KEY_F12: return ImGuiKey_F12;
    //    case GLFW_KEY_F13: return ImGuiKey_F13;
    //    case GLFW_KEY_F14: return ImGuiKey_F14;
    //    case GLFW_KEY_F15: return ImGuiKey_F15;
    //    case GLFW_KEY_F16: return ImGuiKey_F16;
    //    case GLFW_KEY_F17: return ImGuiKey_F17;
    //    case GLFW_KEY_F18: return ImGuiKey_F18;
    //    case GLFW_KEY_F19: return ImGuiKey_F19;
    //    case GLFW_KEY_F20: return ImGuiKey_F20;
    //    case GLFW_KEY_F21: return ImGuiKey_F21;
    //    case GLFW_KEY_F22: return ImGuiKey_F22;
    //    case GLFW_KEY_F23: return ImGuiKey_F23;
    //    case GLFW_KEY_F24: return ImGuiKey_F24;
    //    default: return ImGuiKey_None;
    //    }
    //}

    //bool ImGuiVulkanUtil::getWantKeyCapture() {
    //    return ImGui::GetIO().WantCaptureKeyboard;
    //}

    //void ImGuiVulkanUtil::charPressed(uint32_t key) {
    //    ImGuiIO& io = ImGui::GetIO();
    //    io.AddInputCharacter(key);
    //}

}