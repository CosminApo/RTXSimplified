#ifndef RTX_EXCEPTION_H
#define RTX_EXCEPTION_H

#include <d3d12.h> // DXR -> HRESULT
#include <string> // std::string

namespace RTXSimplified
{

	/**
	*  \brief The class that will manage exceptions for other classes.
	*	
	*/
	class RTX_Exception
	{
	public:
		static void handleError(HRESULT* _hr, std::string _error); ///< Checks for error, displays custom message
		static void handleError(std::string _error, bool _fatal); ///< Displays custom error message, stops program if needed.
	private:
	};
}
#endif // !RTX_EXCEPTION_H