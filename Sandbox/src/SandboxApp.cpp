#include "ZEngine.h"

class TestLayer : public ZEngine::Layer {
public:
	TestLayer()
		: Layer("Test")
	{

	}

	void OnUpdate() override {
		if (ZEngine::Input::IsKeyPressed(ZE_KEY_TAB))
			ZE_TRACE("Tab key is pressed (poll)!");
	}

	void OnEvent(ZEngine::Event& event) override {
		if (event.GetEventType() == ZEngine::EventType::KeyPressed)
		{
			ZEngine::KeyPressedEvent& e = (ZEngine::KeyPressedEvent&)event;
			if (e.GetKeyCode() == ZE_KEY_TAB)
				ZE_TRACE("Tab key is pressed (event)!");
			ZE_TRACE("{0}", (char)e.GetKeyCode());
		}
	}

};

class Game : public ZEngine::Application {
public:
	Game() {
		PushLayer(new TestLayer());
		PushOverlay(new ZEngine::ImGuiLayer());
	}

	~Game() {

	}
};

ZEngine::Application* ZEngine::CreateApplication() {
	return new Game();
}