#include "Renderer2D.h"

#include "Renderer.h"
#include "Shader.h"

namespace ZEngine {

	struct Renderer2DStorage {
		Ref<VertexArray> QuadVertexArray;
		Ref<Shader> FlatShader;
		Ref<PipelineState> QuadPipelineState;
	};

	static Renderer2DStorage* s_Data;

	void Renderer2D::Init() {
		s_Data = new Renderer2DStorage();

		auto& s_LayoutManager = Renderer::GetLayoutManager();
		auto& s_DescriptorAllocator = Renderer::GetDescriptorAllocator();

		// Shader
		s_Data->FlatShader = Shader::Create("Shader", "FlatShader.spv");

		// Buffers and array config
		s_Data->QuadVertexArray = VertexArray::Create();
		float vertices[2 * 4] = {
			// Position
			-0.5f, -0.5f,
			 0.5f, -0.5f,
			 0.5f,  0.5f,
			-0.5f,  0.5f,
		};

		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

		Ref<VertexBuffer> vertexBuffer;
		vertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));

		BufferLayout layout = {
			{ ShaderDataType::Float2, "a_Position" },
		};
		vertexBuffer->SetLayout(layout);

		Ref<IndexBuffer> indexBuffer;
		indexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		s_Data->QuadVertexArray->SetVertexBuffer(vertexBuffer);
		s_Data->QuadVertexArray->SetIndexBuffer(indexBuffer);

		// Pipeline state spec
		PipelineSpecification pipelineSpec{ s_Data->FlatShader, layout, false, false };
		s_Data->QuadPipelineState = PipelineState::Create(pipelineSpec, Renderer::GetLayoutManager());
	}

	void Renderer2D::Shutdown() {
		delete s_Data;
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera) {
		CameraData cameraUBO {};
		cameraUBO.model = glm::mat4(1.0f);
		cameraUBO.view = camera.GetViewMatrix();
		cameraUBO.proj = camera.GetProjectionMatrix();
		Renderer::GetCameraUBO()->SetData(&cameraUBO, sizeof(cameraUBO));
	}

	void Renderer2D::EndScene() {
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color) {
		// Add color to pushconstants
		PushConstantData pushConstants{};
		pushConstants.transform = glm::mat4(1.0f);

		RenderCommand::BindPipelineState(s_Data->QuadPipelineState);
		RenderCommand::BindGlobalSet(s_Data->QuadPipelineState);
		RenderCommand::PushConstant(s_Data->QuadPipelineState, pushConstants);
		RenderCommand::DrawIndexed(s_Data->QuadVertexArray, 0);
	}

}