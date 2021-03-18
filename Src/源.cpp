#include "MyMainWindow.h"


int main()
{
	//vulkan::VulkanSystem::enable(1);
	CraftEngine::gui::Application app;
	auto main_window = new MyMainWindow();
	main_window->setUpdateEveryFrame(true);
	main_window->exec();

}

