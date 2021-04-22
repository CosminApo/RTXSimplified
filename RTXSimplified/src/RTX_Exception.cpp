#include "RTX_Exception.h"
#include <iostream>
#include <exception>

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
	void RTX_Exception::handleError(std::string _error, bool _fatal)
	{
		std::cout << "ERROR: " << _error << std::endl;
		if (_fatal)
		{
			std::cout << "FATAL" << std::endl;
			throw std::exception();
		}
	}
}