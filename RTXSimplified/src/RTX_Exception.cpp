#include "RTX_Exception.h"
#include <iostream>

namespace RTXSimplified
{
	void RTX_Exception::handleError(HRESULT* _hr, std::string _error)
	{
		if (FAILED(*_hr))
		{
			std::cout << "ERROR: " << _error << std::endl;
		}
		*_hr = NULL; //reset HR
	}
}