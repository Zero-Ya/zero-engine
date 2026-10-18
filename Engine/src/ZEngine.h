#pragma once

// For use by game applications

#include "ZEngine/Core/Application.h"
#include "ZEngine/Core/Layer.h"
#include "ZEngine/Core/Log.h"

#include "ZEngine/Core/Timestep.h"

#include "ZEngine/Core/Input.h"
#include "ZEngine/Core/KeyCodes.h"
#include "ZEngine/Core/MouseButtonCodes.h"
#include "ZEngine/Renderer/PerspectiveCameraController.h"
#include "ZEngine/Renderer/OrthographicCameraController.h"

#include "ZEngine/ImGui/ImGuiLayer.h"

// ---RHI-----------------------------
#include "ZEngine/RHI/RHICommandList.h"

// ---Renderer------------------------
//#include "ZEngine/Renderer/Renderer2D.h"

#include "ZEngine/Renderer/Buffer.h"
#include "ZEngine/Renderer/Shader.h"
#include "ZEngine/Renderer/PipelineState.h"
//#include "ZEngine/Renderer/Cubemap.h"

#include "ZEngine/Renderer/ResourceManager.h"
#include "ZEngine/Renderer/BindlessDescriptorManager.h"
#include "ZEngine/Renderer/ModelLoader.h"
#include "ZEngine/Renderer/RenderGraph.h"
#include "ZEngine/Renderer/SceneRenderer.h"

#include "ZEngine/Renderer/PerspectiveCamera.h"
#include "ZEngine/Renderer/OrthographicCamera.h"
// -----------------------------------