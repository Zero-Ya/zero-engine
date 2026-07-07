#pragma once

#ifdef ZE_PLATFORM_WINDOWS

extern ZEngine::Application* ZEngine::CreateApplication();

int main(int argc, char** argv) {
	ZEngine::Log::Init();

	auto game = ZEngine::CreateApplication();
	game->Run();
	delete game;
}

#endif