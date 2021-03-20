#include "RTX_Manager.h"
#include <iostream>
namespace RTXSimplified
{
	int RTX_Manager::initialize()
	{
		self = std::make_shared<RTX_Manager>();
		self.lock()->initializer->setRTXManager(self.lock());
		self.lock()->initializer = std::make_shared<RTX_Initializer>();
		self.lock()->initializer->checkRTXSupport();
		if (self.lock()->initializer->getRTXsupported())
		{
			std::cout << "Yes it is." << std::endl;
		}
		return 0;
	}
	HWND RTX_Manager::getHWND()
	{
		return hwnd;
	}
}