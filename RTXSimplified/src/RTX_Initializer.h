#ifndef RTX_INITIALIZER_H
#define RTX_INITIALIZER_H

#include "d3dx12.h" // DXR
#include <d3d12.h> // DXR
#include <dxgi1_4.h> // DXR
#include <wrl.h> // Windows Runtime Library -> ComPtr
using Microsoft::WRL::ComPtr; ///< Smart pointer for interfaces

namespace RTXSimplified
{
	/**
	*  \brief The class responsible for initializing the library components.
	*
	*	Initializes acceleration structures, raytracing pipeline, shader resource heap
	*	and shader binding table. Also performs checks for RTX support.
	*/
	class RTX_Initializer
	{
	private:
		bool rtxSupported = false; ///< Flag to store whether device supports it.
		ComPtr<ID3D12Device5> rtxDevice; ///< Interface for the GPU.
		int createDevice(); ///< Creates the rtx device interface.
		int getAdapter(IDXGIFactory2* _factory, IDXGIAdapter1** _adapter); ///< Gets the hardware adapter used to create the interface.
	public:
		int checkRTXSupport(); ///< Checks if the hardware supports RTX.
		int createAccelerationStructures(); ///< Creates acceleration structure. Uses BVHmanager.
		int createRaytracingPipeline(); ///< Creates RT pipeline.
		int createShaderResourceHeap(); ///< Creates shader resource heap.
		int createShaderBindingTable(); ///< Creates shader binding table.

		/*GETTERS*/
		bool getRTXsupported();
	};
}

#endif // !RTX_INITIALIZER_H