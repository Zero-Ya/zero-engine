#include "ZEngine/Renderer/RenderCommand.h"

namespace ZEngine {

	std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}