#ifndef RTX_INITIALIZER_H
#define RTX_INITIALIZER_H

#include "d3dx12.h" // DXR
#include <d3d12.h> // DXR
#include <dxgi1_4.h> // DXR
#include <wrl.h> // Windows Runtime Library -> ComPtr
#include <memory> // Smart pointers
#include "RTX_Pipeline.h" // Pipeline generation

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
		std::shared_ptr<RTX_Pipeline> pipeline; ///< An instance for the pipeline generator class.
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
		ComPtr<ID3D12DescriptorHeap> descriptorHeap; ///< Collection of contiguos allocation of descriptors.
		UINT descriptorHeapSize; ///< Stores the size of RTV heap descriptor.
		ComPtr<ID3D12Resource> renderTargets[2]; ///< Stores RTV data for each frame.
		ComPtr<ID3D12CommandAllocator> commandAllocator; ///< Allocations of storage for the GPU.
		ComPtr<ID3D12StateObject> rtStateObject; ///< Stores the state of the pipeline.
		ComPtr<ID3D12StateObjectProperties> rtStateObjectProps; ///< Stores the properties of the state of the pipeline.
		ComPtr<ID3D12Resource> outputResource; ///< Stores the RT output.
		ComPtr<ID3D12RootSignature> rootSignature; ///< Stores the root signature.
		CD3DX12_VIEWPORT viewPort; ///< Stores the view port.
		CD3DX12_RECT scissorRect; ///< Stores a rectangle.
		ComPtr<ID3D12DescriptorHeap> rtvHeap; ///< Stores the heap for the rtv.

		ComPtr<ID3D12GraphicsCommandList5> commandList; /*TBCCCCC*/
		ComPtr<ID3D12Resource> vertexBuffer; /*TBCCCCC*/
		HANDLE fenceEvent; /*TBCCCCC*/
		ComPtr<ID3D12Fence> fence; /*TBCCCCC*/
		UINT64 fenceValue = 0; /*TBCCCCC*/
		ComPtr<ID3D12PipelineState> pipelineState; /*TBCCCCC*/

		int createDevice(); ///< Creates the rtx device interface.
		int getAdapter(IDXGIFactory2* _factory, IDXGIAdapter1** _adapter); ///< Gets the hardware adapter used to create the interface.
		int createCommandQueue(); ///< Creates the command queue with set flags and type.
		int createSwapChain(); ///< Creates the swap chain.
		int createDescriptorHeaps(); ///< Creats the descriptor heap.
		int createFrameResources(); ///< Creats RTV for each frame.
		int createCommandAllocator(); ///< Creates the command allocator.
		int createRTOutput(); ///< Creates the buffer to store the raytracing output.

	public:
		int checkRTXSupport(); ///< Checks if the hardware supports RTX.
		int createPipeline(); ///< Creates the neccessary components for using DX12 DXR.
	///	int createAccelerationStructures(); ///< Creates acceleration structure. Uses BVHmanager.
		int createRaytracingPipeline(); ///< Creates RT pipeline.
		


		/*GETTERS*/
		bool getRTXsupported();
		ComPtr<ID3D12Device5> getRTXDevice();
		ComPtr<ID3D12GraphicsCommandList5> getCommandList();
		ComPtr<ID3D12CommandQueue> getCommandQueue();
		ComPtr<ID3D12CommandAllocator> getCommandAllocator();
		ComPtr<ID3D12Resource> getVertexBuffer();
		ComPtr<ID3D12Fence> getFence();
		ComPtr<ID3D12PipelineState> getPipelineState();
		UINT64 getFenceValue();
		HANDLE getFenceEvent();
		ComPtr<ID3D12Resource> getOutputResource();
		ComPtr<ID3D12StateObjectProperties> getRTStateObjProperties();
		ComPtr<ID3D12RootSignature> getRootSignature();
		CD3DX12_VIEWPORT getViewPort();
		CD3DX12_RECT getScissorRect();
		ComPtr<ID3D12Resource> getRenderTarget();
		ComPtr<ID3D12DescriptorHeap> getRTVheap();
		UINT getFrameIndex();
		UINT getDescriptorHeapSize();
		std::shared_ptr<RTX_Pipeline> getPipeline();
		ComPtr<ID3D12StateObject> getRTStateObject();

		/*SETTERS*/
		void setViewPortHeight(int _height);
		void setViewPortWidth(int _width);
		void setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager);
		void setFenceValue(int _value);
	};
}

#endif // !RTX_INITIALIZER_H