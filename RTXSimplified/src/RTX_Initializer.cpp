#include "RTX_Initializer.h"
#include "RTX_Exception.h"
#include "RTX_Manager.h"


namespace RTXSimplified
{

#pragma region Setters
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

	void RTX_Initializer::setFrameIndex(UINT _value)
	{
		frameIndex = _value;
	}
#pragma endregion

#pragma region Getters
	ComPtr<ID3D12Device5> RTX_Initializer::getRTXDevice()
	{
		return rtxDevice;
	}

	bool RTX_Initializer::getRTXsupported()
	{
		return rtxSupported;
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

	ComPtr<ID3D12Resource> RTX_Initializer::getOutputResource()
	{
		return outputResource;
	}

	ComPtr<ID3D12StateObjectProperties> RTX_Initializer::getRTStateObjProperties()
	{
		return rtStateObjectProps;
	}

	ComPtr<ID3D12RootSignature> RTX_Initializer::getRootSignature()
	{
		return rootSignature;
	}

	CD3DX12_VIEWPORT RTX_Initializer::getViewPort()
	{
		return viewPort;
	}

	CD3DX12_RECT RTX_Initializer::getScissorRect()
	{
		return scissorRect;
	}

	ComPtr<ID3D12Resource> RTX_Initializer::getRenderTarget()
	{
		return renderTargets[frameIndex];
	}

	ComPtr<ID3D12DescriptorHeap> RTX_Initializer::getRTVheap()
	{
		return rtvHeap;
	}

	UINT RTX_Initializer::getFrameIndex()
	{
		return frameIndex;
	}

	UINT RTX_Initializer::getDescriptorHeapSize()
	{
		return descriptorHeapSize;
	}

	std::shared_ptr<RTX_Pipeline> RTX_Initializer::getPipeline()
	{
		return pipeline;
	}

	ComPtr<ID3D12StateObject> RTX_Initializer::getRTStateObject()
	{
		return rtStateObject;
	}

	ComPtr<ID3D12Resource> RTX_Initializer::getCameraBuffer()
	{
		return cameraBuffer;
	}

	uint32_t RTX_Initializer::getCameraBufferSize()
	{
		return cameraBufferSize;
	}

	ComPtr<IDXGISwapChain3> RTX_Initializer::getSwapChain()
	{
		return swapChain;
	}

	ComPtr<ID3D12Resource> RTX_Initializer::getGlobalConstantBuffer()
	{
		return globalConstantBuffer;
	}

	std::vector<ComPtr<ID3D12Resource>> RTX_Initializer::getInstanceBuffers()
	{
		return perInstanceConstantBuffers;
	}
#pragma endregion


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
		createDevice();	// Create the device
		HRESULT hr; // Error handling
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 checker = {}; // query for checking RT, render passes and shader resource view support
		hr = rtxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &checker, sizeof(checker));
		RTX_Exception::handleError(&hr, "Failed to query device"); // Error handling

		if (checker.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		{
			RTX_Exception::handleError("No RTX device found.", true);
		}
		else
		{
			rtxSupported = true;
		}
		return 0;
	}

#pragma endregion

#pragma region RTX_PIPELINE

	int RTX_Initializer::createCamera()
	{
		uint32_t nbMatrix = 4; // Four matrices by default: view, perspective, viewInv, perspectiveInv
		cameraBufferSize = nbMatrix * sizeof(DirectX::XMMATRIX); // Size = matrix number * size of one matrix
		cameraBuffer = pipeline->createBuffer(		// Create the constant buffer for all matrices
			rtxDevice.Get(),						// for this device
			cameraBufferSize,						// this big
			D3D12_RESOURCE_FLAG_NONE,				// no flags
			D3D12_RESOURCE_STATE_GENERIC_READ,		// generic state
			uploadHeapProperties					// default upload properties
		);
		return 0;
	}

	int RTX_Initializer::createCommandQueue()
	{
		HRESULT hr; // Error handling

		D3D12_COMMAND_QUEUE_DESC queueDesc = {}; // Descriptor containing info about the command queue.
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // Set no flags for now. Swap to disable GPU timeout.
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // Type of command list. Direct is default. 

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
			IID_PPV_ARGS(&rtvHeap));				// and store it here

		RTX_Exception::handleError(&hr, "Error creating descriptor heap chain"); // Handle errors	

		descriptorHeapSize = rtxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); // Store its size too

		return 0;
	}

	int RTX_Initializer::createFrameResources()
	{
		HRESULT hr; // Error handling

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart()); // Create a new handle.
		
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

	int RTX_Initializer::createRTOutput()
	{
		HRESULT hr; // Error checking
		/*Sanity check*/
		if (rtxManager->getWidth() == 0 || rtxManager->getHeight() == 0)
		{
			RTX_Exception::handleError("Error creating the RT output buffer: height or width set to 0", false);
		}

		D3D12_RESOURCE_DESC resourceDesc = {};					   			// Descriptor for the RT output buffer:
		resourceDesc.DepthOrArraySize = 1;									// size 1
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		// output alligned same as a texture
		resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;					// RGB 8bit
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;	// no need to force ordered access
		resourceDesc.Width = rtxManager->getWidth();						// use width of window
		resourceDesc.Height = rtxManager->getHeight();						// use height of window
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;					// layout as a texture
		resourceDesc.MipLevels = 1;											// no mipmaps
		resourceDesc.SampleDesc.Count = 1;									// 1 desc
		hr = rtxDevice->CreateCommittedResource(		// Create a new commited resource
			&defaultHeapProperties,						// using default properties
			D3D12_HEAP_FLAG_NONE,						// no extra flags
			&resourceDesc,								// using this descriptor
			D3D12_RESOURCE_STATE_COPY_SOURCE,			// copy source
			nullptr,									// must be nullptr for buffers
			IID_PPV_ARGS(&outputResource)				// store it here
		);
		RTX_Exception::handleError(&hr, "Error creating output resource."); // Handle errors

		return 0;
	}

	int RTX_Initializer::createPipelineState()
	{
		HRESULT hr; // Error handling.

		// Store the two shaders here
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

		// Need to convert string to lpcwstr first
		std::wstring temp = pipeline->stringToWstring(rtxManager->getMainShader());
		LPCWSTR convertedName = temp.c_str();

		// Compile the vertex shader
		hr = D3DCompileFromFile(
			convertedName,			// file name
			nullptr,				// no defines
			nullptr,				// no includes
			"VSMain",				// start from here
			"vs_5_0",				// target
			compileFlags,			// flags
			0,						// no extra flags
			&vertexShader,			// store here
			nullptr					// no extra error msg storage
		);
		RTX_Exception::handleError(&hr, "Error compiling vertex shader."); // Handle errors

		// Compile the vertex shader
		hr = D3DCompileFromFile(
			convertedName,			// file name
			nullptr,				// no defines
			nullptr,				// no includes
			"PSMain",				// start from here
			"ps_5_0",				// target
			compileFlags,			// flags
			0,						// no extra flags
			&pixelShader,			// store here
			nullptr					// no extra error msg storage
		);
		RTX_Exception::handleError(&hr, "Error compiling fragment shader."); // Handle errors

		// Define the vertex input layout (matches the vertex struct)
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO)
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};							// Stores the properties
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };	// define the layout
		psoDesc.pRootSignature = rootSignature.Get();								// use the root signature created
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());					// use the vertex shader compiled
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());					// use the pixel shader compiled
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);			// default rasterizer state
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);						// default blend state
		psoDesc.DepthStencilState.DepthEnable = FALSE;								// no depth testing yet
		psoDesc.DepthStencilState.StencilEnable = FALSE;							// no stencil yet
		psoDesc.SampleMask = UINT_MAX;												// max amount of smaples
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		// based arount triangles
		psoDesc.NumRenderTargets = 1;												// one render target
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;							// RGB format
		psoDesc.SampleDesc.Count = 1;												// one sample.

		hr = rtxDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)); // Create the pipeline
		RTX_Exception::handleError(&hr, "Error creating the pipeline state."); // Handle errors
		return 0;
	}

	int RTX_Initializer::createFence()
	{
		HRESULT hr; // Error handling

		// Create basic fence
		hr = rtxDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)); 
		RTX_Exception::handleError(&hr, "Error creating the fence."); // Handle error
		fenceValue = 1;
		// Create an event handle to use for frame synchronization.
		fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fenceEvent == nullptr)
		{
			RTX_Exception::handleError("Error creating fence event.", true); // Handle errors
		}

		// Wait for the command list to execute
		rtxManager->waitForPreviousFrame();
		return 0;
	}

	int RTX_Initializer::createRaytracingPipeline()
	{
		HRESULT hr; // Error handling
		// Set up hierarchy.
		pipeline = std::make_shared<RTX_Pipeline>();
		pipeline->setRTXManager(rtxManager);
		pipeline->createDefaultRootSignature(); // Create the default global and local signatures
		pipeline->createShaderLibraries(); // Read and compile shaders
		pipeline->addLibraries(); // Store the shader libs
		pipeline->createShaderSignatures();	// Create the signatures for the shaders
		/*Add three Hit groups: one for the plane, one main and one shadow (optional)*/
		pipeline->addHitGroup(L"HitGroup", L"ClosestHit");
		pipeline->addHitGroup(L"PlaneHitGroup", L"PlaneClosestHit");
		if (rtxManager->getShadowsEnabled())
		{
			pipeline->addHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");
		}
		pipeline->addRootSignatureAssociation(); // Create the root signatures
		pipeline->setMaxPayloadSize(4 * sizeof(float)); // RGB + distance
		pipeline->setMaxAttributeSize(2 * sizeof(float)); // XY
		if (rtxManager->getShadowsEnabled())
		{
			pipeline->setMaxRecusionDepth(2); // Primary rays and shadow rays
		}
		else
		{
			pipeline->setMaxRecusionDepth(1); // Primary rays
		}
		rtStateObject = pipeline->generate(); // Generate the pipeline
		hr = rtStateObject->QueryInterface(IID_PPV_ARGS(&rtStateObjectProps)); // Generate the properties
		RTX_Exception::handleError(&hr, "Error creating pipeline properties."); // Handle errors
		return 0;
	}

	int RTX_Initializer::prepareAssetLoading()
	{
		/* Initialize values*/
		frameIndex = 0;
		viewPort = CD3DX12_VIEWPORT(
			0.0f,									// x
			0.0f,									// y
			static_cast<float>(viewPort_width),
			static_cast<float>(viewPort_height)
		);
		scissorRect = CD3DX12_RECT(
			0.0f,									// x
			0.0f,									// y
			static_cast<float>(viewPort_width),
			static_cast<float>(viewPort_height)
		);

		HRESULT hr; // Error handling

		// Create a default root signature.
		CD3DX12_ROOT_PARAMETER constantParameter;
		CD3DX12_DESCRIPTOR_RANGE range;
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		constantParameter.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(1, &constantParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature; // Stores the root signature.
		ComPtr<ID3DBlob> error;

		hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		RTXSimplified::RTX_Exception::handleError(&hr, "Error serializing root signature."); // handle errors
		hr = rtxDevice->CreateRootSignature(		// Create the signature
			0,										// no params
			signature->GetBufferPointer(),			// from here
			signature->GetBufferSize(),				// this size
			IID_PPV_ARGS(&rootSignature));			// store here 
		return 0;
	}

	int RTX_Initializer::updateCameraBuffer()
	{
		std::vector<DirectX::XMMATRIX> matrices(4); // Initialize the view matrix
		DirectX::XMVECTOR eye = DirectX::XMVectorSet(1.5f, 1.5f, 1.5f, 0.0f); // Looking from here
		DirectX::XMVECTOR at = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f); // towards this
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // with up being this way
		matrices[0] = DirectX::XMMatrixLookAtRH(eye, at, up); // Create view matrix
		float fovAngleY = 45.0f * DirectX::XM_PI / 180.0f;
		matrices[1] = DirectX::XMMatrixPerspectiveFovRH(fovAngleY, 1.5, 0.1f, 1000.0f); // Create perspective matrix. Raytracing has to do the contrary of rasterization: rays are defined in 
																						// camera space, and are transformed into world space 
		DirectX::XMVECTOR det;
		matrices[2] = XMMatrixInverse(&det, matrices[0]); // Create view Inverted matrix
		matrices[3] = XMMatrixInverse(&det, matrices[1]); // Create perspective Inverted matrix
		// Copy the matrix contents 
		uint8_t* pData;
		HRESULT hr = cameraBuffer->Map(0, nullptr, (void**)&pData);
		RTX_Exception::handleError(&hr, "Error updating camera");
		memcpy(pData, matrices.data(), cameraBufferSize);
		cameraBuffer->Unmap(0, nullptr);

		return 0;
	}

	int RTX_Initializer::createGlobalConstantBuffer()
	{
		DirectX::XMVECTOR bufferData[] = {
			DirectX::XMVECTOR{1.0f, 0.0f, 0.0f, 1.0f},
			DirectX::XMVECTOR{0.7f, 0.4f, 0.0f, 1.0f},
			DirectX::XMVECTOR{0.4f, 0.7f, 0.0f, 1.0f},
			DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 1.0f},
			DirectX::XMVECTOR{0.0f, 0.7f, 0.4f, 1.0f},
			DirectX::XMVECTOR{0.0f, 0.4f, 0.7f, 1.0f}, 
			DirectX::XMVECTOR{0.0f, 0.0f, 1.0f, 1.0f},
			DirectX::XMVECTOR{0.4f, 0.0f, 0.7f, 1.0f}, 
			DirectX::XMVECTOR{0.7f, 0.0f, 0.4f, 1.0f}, }; 
		// Create the buffer
		globalConstantBuffer = pipeline->createBuffer( // Create a buffer
			rtxDevice.Get(),						   // for this device
			sizeof(bufferData),						   // this big
			D3D12_RESOURCE_FLAG_NONE,				   // no flags
			D3D12_RESOURCE_STATE_GENERIC_READ,		   // generic state
			uploadHeapProperties					   // default upload properties
		);
		// Copy CPU memory to GPU
		uint8_t* pData; 
		HRESULT hr = globalConstantBuffer->Map(0, nullptr, (void**)&pData);
		memcpy(pData, bufferData, sizeof(bufferData));
		globalConstantBuffer->Unmap(0, nullptr);
		return 0;
	}

	void RTX_Initializer::createPerInstanceConstantBuffers()
	{
		// Initialize with 9 default XMVECTOR 4 values because of HLSL packing
		DirectX::XMVECTOR bufferData[] = {
			DirectX::XMVECTOR{1.0f, 0.0f, 0.0f, 1.0f},
			DirectX::XMVECTOR{1.0f, 0.4f, 0.0f, 1.0f},
			DirectX::XMVECTOR{1.f, 0.7f, 0.0f, 1.0f},
			DirectX::XMVECTOR{0.0f, 1.0f, 0.0f, 1.0f},
			DirectX::XMVECTOR{0.0f, 1.0f, 0.4f, 1.0f},
			DirectX::XMVECTOR{0.0f, 1.0f, 0.7f, 1.0f},
			DirectX::XMVECTOR{0.0f, 0.0f, 1.0f, 1.0f},
			DirectX::XMVECTOR{0.4f, 0.0f, 1.0f, 1.0f},
			DirectX::XMVECTOR{0.7f, 0.0f, 1.0f, 1.0f}, };

		perInstanceConstantBuffers.resize(3);

		int i(0);

		for (auto& cb : perInstanceConstantBuffers) // For each buffer
		{
			const uint32_t bufferSize = sizeof(DirectX::XMVECTOR) * 3; // Set the size to three vectors
			cb = pipeline->createBuffer(			// Create the buffer
				rtxDevice.Get(),					// for this device
				bufferSize,							// this big
				D3D12_RESOURCE_FLAG_NONE,			// no flags
				D3D12_RESOURCE_STATE_GENERIC_READ,	// generic state
				uploadHeapProperties
			);
			//Copy the data over to the GPU
			uint8_t* pData;
			HRESULT hr = (cb->Map(0, nullptr, (void**)&pData));
			RTX_Exception::handleError(&hr, "Error creating instance buffer");
			memcpy(pData, &bufferData[i * 3], bufferSize);
			cb->Unmap(0, nullptr);
			++i;
		}
	}


	int RTX_Initializer::createPipeline()
	{
		HRESULT hr; // Error handling

		createCommandAllocator();
		createPipelineState();
		createCommandQueue();
		createSwapChain();
		createDescriptorHeaps();
		createFrameResources();
		createRTOutput();
		createFence();
		
	   // Create command list
		hr = rtxDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocator.Get(),
			pipelineState.Get(),
			IID_PPV_ARGS(&commandList)
		);
		RTXSimplified::RTX_Exception::handleError(&hr, "Error creating command list.");

		return 0;
	}

#pragma endregion




}