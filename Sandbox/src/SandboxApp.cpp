#include "ZEngine.h"

class TestLayer : public ZEngine::Layer {
public:
	TestLayer()
		: Layer("Test")
	{

	}

	void OnUpdate() override {

	}

	void OnEvent(ZEngine::Event& event) override {

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