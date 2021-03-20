#ifndef RTX_INITIALIZER_H
#define RTX_INITIALIZER_H

#include "d3dx12.h" // DXR
#include <d3d12.h> // DXR
#include <dxgi1_4.h> // DXR
#include <wrl.h> // Windows Runtime Library -> ComPtr
#include <memory> // Smart pointers

using Microsoft::WRL::ComPtr; ///< Smart pointer for interfaces

namespace RTXSimplified
{
	/*FORWARD DECLARES*/
	class RTX_Manager;

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
		ComPtr<ID3D12CommandQueue> commandQueue; ///< Stores the command queue, which stores command lists to be executed.
												 ///< Associated with device.
		ComPtr<IDXGIFactory4> factory; ///< Interface for creating DXGI objects
		ComPtr<IDXGISwapChain3> swapChain; ///< Handles stored render outputs.
		int viewPort_width; ///< Stores properties for the viewport to output to.
		int viewPort_height; ///< Stores properties for the viewport to output to.
		std::shared_ptr<RTX_Manager> rtxManager; ///< Store a reference to the RTX manager class.
		UINT frameIndex; ///< Used for synchronization.

		int createDevice(); ///< Creates the rtx device interface.
		int getAdapter(IDXGIFactory2* _factory, IDXGIAdapter1** _adapter); ///< Gets the hardware adapter used to create the interface.
		int createCommandQueue(); ///< Creates the command queue with set flags and type.
		int createSwapChain(); ///< Creates the swap chain.

	public:
		int checkRTXSupport(); ///< Checks if the hardware supports RTX.
		int createPipeline(); ///< Creates the neccessary components for using DX12 DXR.
		int createAccelerationStructures(); ///< Creates acceleration structure. Uses BVHmanager.
		int createRaytracingPipeline(); ///< Creates RT pipeline.
		int createShaderResourceHeap(); ///< Creates shader resource heap.
		int createShaderBindingTable(); ///< Creates shader binding table.

		/*GETTERS*/
		bool getRTXsupported();
	

		/*SETTERS*/
		void setViewPortHeight(int _height);
		void setViewPortWidth(int _width);
		void setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager);
	};
}

#endif // !RTX_INITIALIZER_H