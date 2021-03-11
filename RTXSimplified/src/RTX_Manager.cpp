#include "RTX_Manager.h"
#include <iostream>
namespace RTXSimplified
{
	int RTX_Manager::initialize()
	{
		initializer = std::make_shared<RTX_Initializer>();
		initializer->checkRTXSupport();
		if (initializer->getRTXsupported())
		{
			std::cout << "Yes it is." << std::endl;
		}
		return 0;
	}
}