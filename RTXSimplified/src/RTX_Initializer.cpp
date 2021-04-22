#include "RTX_Initializer.h"
#include "RTX_Exception.h"
#include "RTX_Manager.h"


namespace RTXSimplified
{
void RTX_Initializer::setViewPortHeight(int _height)
{
	viewPort_height = _height;
}
void RTX_Initializer::setViewPortWidth(int _width)
{
	viewPort_width = _width;
}

void RTX_Initializer::setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager)
{
	rtxManager = _rtxManager;
}

void RTX_Initializer::setFenceValue(int _value)
{
	fenceValue = _value;
}

ComPtr<ID3D12Device5> RTX_Initializer::getRTXDevice()
{
	return rtxDevice;
}

ComPtr<ID3D12GraphicsCommandList5> RTX_Initializer::getCommandList()
{
	return commandList;
}

ComPtr<ID3D12CommandQueue> RTX_Initializer::getCommandQueue()
{
	return commandQueue;
}

ComPtr<ID3D12CommandAllocator> RTX_Initializer::getCommandAllocator()
{
	return commandAllocator;
}

ComPtr<ID3D12Resource> RTX_Initializer::getVertexBuffer()
{
	return vertexBuffer;
}

ComPtr<ID3D12Fence> RTX_Initializer::getFence()
{
	return fence;
}

ComPtr<ID3D12PipelineState> RTX_Initializer::getPipelineState()
{
	return pipelineState;
}

UINT64 RTX_Initializer::getFenceValue()
{
	return fenceValue;
}

HANDLE RTX_Initializer::getFenceEvent()
{
	return fenceEvent;
}

#pragma region RTX_SUPPORT_CHECK
	int RTX_Initializer::createDevice()
	{
		HRESULT hr; // Error handling
		UINT dxgiFactoryFlags = 0; // Stores flags for creating factory
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG; // Enable additional debug layers		
		ComPtr<IDXGIAdapter1> hardwareAdapter; // Represents a GPU

		hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)); // Create the factory
		RTX_Exception::handleError(&hr, "Failed to create factory"); // Error handling

		getAdapter(factory.Get(), &hardwareAdapter); // Get the GPU

		hr = D3D12CreateDevice(			// Creates the device:
			hardwareAdapter.Get(),		// GPU to use
			D3D_FEATURE_LEVEL_12_1,		// Minimum feature level
			IID_PPV_ARGS(&rtxDevice)	// Where to store output
		);
		RTX_Exception::handleError(&hr, "Failed to create device"); // Error handling

		return 0;
	}

	int RTX_Initializer::getAdapter(IDXGIFactory2* _factory, IDXGIAdapter1** _adapter)
	{
		ComPtr<IDXGIAdapter1> adapter; // Store the adapter temp
		*_adapter = nullptr; // Set the other adapter to null if nothing found

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != _factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex) // Loop through all the adapters found
		{
			DXGI_ADAPTER_DESC1 desc; // Get the description for each adaptor
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) // Don't select the Basic Render Driver adapter.
			{
				continue;
			}
			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		*_adapter = adapter.Detach(); // Detach the adapter for cleanup
		return 0;
	}

	int RTX_Initializer::checkRTXSupport()
	{
		createDevice();
		HRESULT hr; // Error handling
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 checker = {}; // query for checking RT, render passes and shader resource view support
		hr = rtxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &checker, sizeof(checker));
		RTX_Exception::handleError(&hr, "Failed to query device"); // Error handling


		if (checker.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		{
			// HARDWARE NOT SUPPORTED
		}
		else
		{
			rtxSupported = true;
		}
		return 0;
	}
	bool RTX_Initializer::getRTXsupported()
	{
		return rtxSupported;
	}


#pragma endregion

#pragma region RTX_PIPELINE

	int RTX_Initializer::createCommandQueue()
	{
		HRESULT hr; // Error handling

		D3D12_COMMAND_QUEUE_DESC queueDesc = {}; // Descriptor containing info about the command queue.
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // Set no flags for now. Swap to disable GPU timeout.
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // Type of command list. Direct is default. https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_command_list_type

		hr = rtxDevice->CreateCommandQueue(		// Create a command queue
			&queueDesc,							// With that descriptor
			IID_PPV_ARGS(&commandQueue)),		// And store it here

		RTX_Exception::handleError(&hr, "Error creating command queue"); // Handle errors	


		return 0;
	}

	int RTX_Initializer::createSwapChain()
	{
		HRESULT hr; // Error handling

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {}; // Descriptor for the swap chain.
		swapChainDesc.BufferCount = 2;								// Store 2 buffers by default.
		swapChainDesc.Width = viewPort_width;						// with the height of the viewport
		swapChainDesc.Height = viewPort_height;						// and the width of the viewport
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// on a standard rgb format
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// and transition by discarding
		swapChainDesc.SampleDesc.Count = 1;							// 1 sample per pixel.

		ComPtr<IDXGISwapChain1> swapChain1; // Temp.
		hr = factory->CreateSwapChainForHwnd(		// Create a new swap chain
			commandQueue.Get(),						// based on this queue
			rtxManager->getHWND(),					// on this window
			&swapChainDesc,							// using info from this descriptor
			nullptr,								// no additional full screen descriptor
			nullptr,								// no restriction on output
			&swapChain1								// and store the result
			);
		RTX_Exception::handleError(&hr, "Error creating swap chain"); // Handle errors	

		swapChain1.As(&swapChain); // Store the swap chain.
		frameIndex = swapChain->GetCurrentBackBufferIndex(); // Store the current frame index.

		return 0;
	}

	int RTX_Initializer::createDescriptorHeaps()
	{
		HRESULT hr; // Error handling

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {}; // Descriptor for creating the descriptor heap
		rtvHeapDesc.NumDescriptors = 2;								// Set to 2 by default
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;			// the descriptor heap for the render-target view.
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;		// no extra flags

		hr = rtxDevice->CreateDescriptorHeap(		// Create a new descriptor heap
			&rtvHeapDesc,							// basd on this info
			IID_PPV_ARGS(&descriptorHeap));			// and store it here

		RTX_Exception::handleError(&hr, "Error creating descriptor heap chain"); // Handle errors	

		descriptorHeapSize = rtxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); // Store its size too

		return 0;
	}

	int RTX_Initializer::createFrameResources()
	{
		HRESULT hr; // Error handling

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart()); // Create a new handle.
		
		for (int i = 0; i < 2; i++)
		{
			hr = swapChain->GetBuffer(				// Get buffer			
				i,									// at this postion
				IID_PPV_ARGS(&renderTargets[i])		// and store it here
			);
			RTX_Exception::handleError(&hr, "Error getting buffers"); // Handle errors	

			rtxDevice->CreateRenderTargetView(			// Create a new render target view
				renderTargets[i].Get(),					// from this render target
				nullptr,								// a view that accesses all of the subresources in mipmap level 0
				rtvHandle								// using this info
			);
			rtvHandle.Offset(1, descriptorHeapSize); // Ensure you get the next one
		}

		return 0;
	}

	int RTX_Initializer::createCommandAllocator()
	{
		HRESULT hr; // Error handling
		hr = rtxDevice->CreateCommandAllocator(				// Create a new command allocator
			D3D12_COMMAND_LIST_TYPE_DIRECT,					// a command allocator that the GPU can execute. A direct command list doesn't inherit any GPU state.
			IID_PPV_ARGS(&commandAllocator)					// and store it here
		);
		RTX_Exception::handleError(&hr, "Error creating command allocator"); // Handle errors	

		return 0;
	}

	int RTX_Initializer::createRaytracingPipeline()
	{
		HRESULT hr; // Error handling
		// Set up hierarchy.
		std::shared_ptr<RTX_Pipeline> pipeline = std::make_shared<RTX_Pipeline>();
		pipeline->setRTXManager(rtxManager);

		pipeline->createDefaultRootSignature();
		pipeline->createShaderLibraries();		 
		pipeline->addLibraries();
		pipeline->createShaderSignatures();
		pipeline->addHitGroup(L"HitGroup", L"ClosestHit");
		pipeline->addRootSignatureAssociation();
		pipeline->setMaxPayloadSize(4 * sizeof(float)); // RGB + distance
		pipeline->setMaxAttributeSize(2 * sizeof(float)); // XY
		pipeline->setMaxRecusionDepth(1); // only primary rays for now
		rtStateObject = pipeline->generate(); // Generate the pipeline
		hr = rtStateObject->QueryInterface(IID_PPV_ARGS(&rtStateObjectProps)); // Generate the properties.

		return 0;
	}

	int RTX_Initializer::createPipeline()
	{
		createCommandQueue();
		createSwapChain();
		createDescriptorHeaps();
		createFrameResources();
		createCommandAllocator();

		///CREATE OCMMAND LIST


		return 0;
	}



#pragma endregion




}