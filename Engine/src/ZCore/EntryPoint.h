#pragma once

#ifdef ZE_PLATFORM_WINDOWS

extern ZEngine::Application* ZEngine::CreateApplication();

int main(int argc, char** argv) {
	ZEngine::Log::Init();
	ZE_CORE_WARN("Initialized Log!");
	int a = 5;
	ZE_INFO("Hello! Var={0}", a);

	auto game = ZEngine::CreateApplication();
	game->Run();
	delete game;
}

#endif