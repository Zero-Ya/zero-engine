#pragma once

#include "Core.h"

namespace ZEngine {

	class ZE_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}