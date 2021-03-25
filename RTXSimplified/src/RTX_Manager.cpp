#include "RTX_Manager.h"
#include <iostream>
namespace RTXSimplified
{
	std::shared_ptr<RTX_Manager> RTX_Manager::initialize()
	{
		std::shared_ptr<RTX_Manager> rtn = std::make_shared<RTX_Manager>();
		rtn->self = rtn;
		rtn->initializer = std::make_shared<RTX_Initializer>();
		rtn->initializer->setRTXManager(rtn);
		rtn->initializer->checkRTXSupport();
		if (rtn->initializer->getRTXsupported())
		{
			std::cout << "Yes it is." << std::endl;
		}

		rtn->bvhManager = std::make_shared<RTX_BVHmanager>();
		rtn->bvhManager->setRTXManager(rtn);

		return rtn;
	}
	std::shared_ptr<RTX_Initializer> RTX_Manager::getInitializer()
	{
		return initializer;
	}
	HWND RTX_Manager::getHWND()
	{
		return hwnd;
	}
}