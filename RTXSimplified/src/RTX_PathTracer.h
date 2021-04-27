#ifndef RTX_PATHTRACER_H
#define RTX_PATHTRACER_H

#include "d3dx12.h" // DirectX12 api
#include <vector> // std::vectore
#include "d3dx12.h" // DXR
#include <d3d12.h> // DXR
#include <dxgi1_4.h> // DXR
#include <memory> // smart pointers
#include <dxcapi.h> //DXR

namespace RTXSimplified
{
	/*Forward declares*/
	class RTX_Manager;
	/**
	*	\brief The class responsible for tracing the path of light rays.
	* 	
	*/
	class RTX_PathTracer
	{
	private:
		std::shared_ptr<RTX_Manager> rtxManager; ///< Stores a reference to the RTX manager class.

	public:
		int populateCommandList(); ///< Populates the command list for execution.

		/*SETTERS*/
		void setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager);
	};
}

#endif // !RTX_PATHTRACER_H
