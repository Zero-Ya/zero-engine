#include "ZCore.h"

class Game : public ZEngine::Application {
public:
	Game() {

	}

	~Game() {

	}
};

ZEngine::Application* ZEngine::CreateApplication() {
	return new Game();
}