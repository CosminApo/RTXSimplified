#include "RTX_Exception.h"
#include <iostream>
#include <exception>

namespace RTXSimplified
{
	void RTX_Exception::handleError(HRESULT* _hr, std::string _error)
	{
		if (*_hr != S_OK) // If its not OK
		{
			std::cout << "ERROR: " << _error << std::endl; // Output the error.
			throw std::exception();	// Stop the program
		}
		*_hr = S_FALSE;
	}
	void RTX_Exception::handleError(std::string _error, bool _fatal)
	{
		// Function is only called when error occurs.

		std::cout << "ERROR: " << _error << std::endl; // Output error
		if (_fatal) //If its a fatal error
		{
			std::cout << "FATAL" << std::endl;
			throw std::exception(); // Stop the program
		}
	}
}