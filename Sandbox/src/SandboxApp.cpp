#include "ZCore.h"

class TestLayer : public ZEngine::Layer {
public:
	TestLayer()
		: Layer("Test")
	{

	}

	void OnUpdate() override {
		ZE_INFO("TestLayer::Update");
	}

	void OnEvent(ZEngine::Event& event) override {
		ZE_TRACE("{0}", event);
	}

};

class Game : public ZEngine::Application {
public:
	Game() {
		PushLayer(new TestLayer());
	}

	~Game() {

	}
};

ZEngine::Application* ZEngine::CreateApplication() {
	return new Game();
}