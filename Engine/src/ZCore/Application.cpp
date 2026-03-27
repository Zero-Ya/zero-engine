#include "Application.h"

#include "ZCore/Events/ApplicationEvent.h"
#include "ZCore/Log.h"

namespace ZEngine {


	Application::Application() {

	}

	Application::~Application() {

	}

	void Application::Run() {
		WindowResizeEvent e(1280, 720);
		if (e.IsInCategory(EventCategoryApplication)) {
			ZE_TRACE(e);
		}
		if (e.IsInCategory(EventCategoryInput)) {
			ZE_TRACE(e);
		}

		while (true);
	}

}