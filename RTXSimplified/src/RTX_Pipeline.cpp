#include "RTX_Pipeline.h"
#include "RTX_Initializer.h"
#include "RTX_Manager.h"
#include "RTX_BVHmanager.h"
#include <fstream>

namespace RTXSimplified
{
	int RTX_Pipeline::addHitGroup(const std::wstring& _hitGroupName, const std::wstring& _closestHitSymbol, const std::wstring& _anyHitSymbol, const std::wstring& _intersectionSymbol)
	{
		hitgroups.emplace_back(HitGroup(_hitGroupName, _closestHitSymbol, _anyHitSymbol, _intersectionSymbol));
		return 0;
	}
	int RTX_Pipeline::addRootSignatureAssociation()
	{
		addRootSignatureAssociation(rayGenSignature.Get(), { L"RayGen" });
		addRootSignatureAssociation(missSignature.Get(), { L"Miss" });
		addRootSignatureAssociation(hitSignature.Get(), { L"Hit" });
		return 0;
	}
	int RTX_Pipeline::addRootSignatureAssociation(ID3D12RootSignature* _rootSig, const std::vector<std::wstring>& _symbols)
	{
		rootSigAssociations.emplace_back(RootSignatureAssociation(_rootSig, _symbols));
		return 0;
	}
	ID3D12DescriptorHeap* RTX_Pipeline::createDescriptorHeap(uint32_t _count, D3D12_DESCRIPTOR_HEAP_TYPE _type, bool _shaderVisible)
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptor = {}; // The descriptor for the desc heap.
		/*Pass in parameters*/
		descriptor.NumDescriptors = _count;
		descriptor.Type = _type;
		descriptor.Flags = _shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ID3D12DescriptorHeap* heap;
		rtxManager->getInitializer()->getRTXDevice()->CreateDescriptorHeap(&descriptor, IID_PPV_ARGS(&heap)); // Create a new heap.

		return heap;
	}
	ID3D12Resource* RTX_Pipeline::createBuffer(ID3D12Device* _device, uint64_t _size, D3D12_RESOURCE_FLAGS _flags, D3D12_RESOURCE_STATES _initState, const D3D12_HEAP_PROPERTIES& _heapProps)
	{
		HRESULT hr; // Error handling

		D3D12_RESOURCE_DESC bufferDesc = {};						// Contains info about how to create the buffer:
		bufferDesc.Alignment = 0;									// must be 64KB or 0 (which is default) for buffer
		bufferDesc.DepthOrArraySize = 1;							// must be 1 for buffer
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// specify its a buffer
		bufferDesc.Flags = _flags;									// D3D12_RESOURCE_FLAGS
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;					// unknown format at this point
		bufferDesc.Height = 1;										// must be 1 for buffer
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// must be for buffer
		bufferDesc.MipLevels = 1;									// must be 1 for buffer
		bufferDesc.SampleDesc.Count = 1;							// must be 1 for buffer
		bufferDesc.SampleDesc.Quality = 0;							// must be 0 for buffer
		bufferDesc.Width = _size;									// how long the memory block is

		ID3D12Resource* buffer;
		hr = rtxManager->getInitializer()->getRTXDevice().Get()->CreateCommittedResource( //Creates both a resource and an implicit heap and the resource is mapped to the heap
			&_heapProps,					// D3D12_HEAP_PROPERTIES for the heap
			D3D12_HEAP_FLAG_NONE,			// D3D12_HEAP_FLAGS for the heap
			&bufferDesc,					// descriptor for buffer
			_initState,						// D3D12_RESOURCE_STATES 
			nullptr,						// dimension buffer needs nullptr
			IID_PPV_ARGS(&buffer)			// store it here
		);
		RTX_Exception::handleError(&hr, "Error creating buffer"); // Handle errors	

		return buffer;
	}
	int RTX_Pipeline::buildShaderExportList(std::vector<std::wstring>& _exportedSymbols)
	{
		std::unordered_set<std::wstring> exports; // Stores all the names from libs and hjitgroups
		
		/* Add all the libs */
		for (const Library& lib : libraries)
		{
			for (const auto& exportName : lib.symbols)
			{
				exports.insert(exportName);
			}
		}

		/* Add al the hitgroups. Note empty symbols first. */
		for (const auto& hitGroup : hitgroups)
		{
			if (!hitGroup.anyHitSymbol.empty())
			{
				exports.erase(hitGroup.anyHitSymbol);
			}
			if (!hitGroup.closestHitSymbol.empty())
			{
				exports.erase(hitGroup.closestHitSymbol);
			}
			if (!hitGroup.intersectionSymbol.empty())
			{
				exports.erase(hitGroup.intersectionSymbol);
			}
			exports.insert(hitGroup.hitGroupName);
		}


		/* Add everything together */
		for (const auto& name : exports)
		{
			_exportedSymbols.push_back(name);
		}

		return 0;
	}
	int RTX_Pipeline::createDefaultRootSignature()
	{
		HRESULT hr; // Error Handling.
		
		D3D12_ROOT_SIGNATURE_DESC rootDescriptor = {}; // Contains info for building a root signature. The root signature 
													   // defines what types of resources are bound to the graphics pipeline.
		rootDescriptor.NumParameters = 0;							 // Add 0 slots for now.
		rootDescriptor.pParameters = nullptr;						 // 0 slots = empty signature = null
		rootDescriptor.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;		 // none = global
		ID3DBlob* serializedRootSignature;	// Generic data buffer storing root rignature.
		ID3DBlob* error;					// Generic data buffer storing errors. NOTE: not used, cause RTX_Exception, but function needs it.

		/*CREATE GLOBAL SIGNATURE*/
		hr = D3D12SerializeRootSignature(		// Serialize a new root signature
			&rootDescriptor,					// using the descriptor created
			D3D_ROOT_SIGNATURE_VERSION_1,		// using version 1.0 since they are empty anyway so the faster 1.1 is not needed.
			&serializedRootSignature,			// store the result here
			&error								// store any errors here
		);
		RTX_Exception::handleError(&hr, "Error serializing global root signature"); // Error handling

		hr = rtxManager->getInitializer()->getRTXDevice()->CreateRootSignature( // Create a new root signature
			0,																	// 0 cause single GPU
			serializedRootSignature->GetBufferPointer(),						// using the previous serialized result
			serializedRootSignature->GetBufferSize(),							//
			IID_PPV_ARGS(&defaultGlobalSignature)								// store the result in the class
		);
		RTX_Exception::handleError(&hr, "Error creating the global root signature"); //Error handling

		serializedRootSignature->Release(); // Empty blob

		/*CREATE LOCAL SIGNATURE*/
		rootDescriptor.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE; // Change the flag to local
		hr = D3D12SerializeRootSignature(		// Serialize a new root signature
			&rootDescriptor,					// using the descriptor created
			D3D_ROOT_SIGNATURE_VERSION_1,		// using version 1.0 since they are empty anyway so the faster 1.1 is not needed.
			&serializedRootSignature,			// store the result here
			&error								// store any errors here
		);
		RTX_Exception::handleError(&hr, "Error serializing local root signature"); // Error handling
		hr = rtxManager->getInitializer()->getRTXDevice()->CreateRootSignature( // Create a new root signature
			0,																	// 0 cause single GPU
			serializedRootSignature->GetBufferPointer(),						// using the previous serialized result
			serializedRootSignature->GetBufferSize(),							//
			IID_PPV_ARGS(&defaultLocalSignature)								// store the result in the class
		);
		RTX_Exception::handleError(&hr, "Error creating the local root signature"); //Error handling

		serializedRootSignature->Release(); // Empty blob
		error->Release(); // Empty blob

		return 0;
	}
	IDxcBlob* RTX_Pipeline::compileShaderLib(std::string _shaderFile)
	{
		HRESULT hr; // Error Handling.
		if (!compiler) // Only do this if compiler has not been created yet.
		{
			hr = DxcCreateInstance(			// Create a new instance
				CLSID_DxcCompiler,			// of type compiler
				__uuidof(IDxcCompiler),		// based on this data type
				(void**)&compiler			// store the result here
			);
			RTX_Exception::handleError(&hr, "Error creating the shader compiler."); // Error handling
		}
		if (!library) // Only do this if library has not been created yet.
		{
			hr = DxcCreateInstance(			// Create a new instance
				CLSID_DxcCompiler,			// of type library
				__uuidof(IDxcCompiler),		// based on this data type
				(void**)&compiler			// store the result here
			);
			RTX_Exception::handleError(&hr, "Error creating the shader library."); // Error handling
		}
		if (!includeHandler) // Only do this if handler has not been created yet.
		{
			hr = library->CreateIncludeHandler(&includeHandler); // Create the handler based on the library.
			RTX_Exception::handleError(&hr, "Error creating the shader handler."); // Error handling
		}

		std::ifstream file(_shaderFile); // Open the file
		std::string line = "";
		std::string shader = "";
		if (!file.is_open()) // Error handle
		{
			RTX_Exception::handleError("Error opening the shader file: " + _shaderFile, true);
		}
		while (std::getline(file, line)) // Read the file in
		{
			shader = shader + line;
		}

		IDxcBlobEncoding* textBlob;	// String needs to be converted to blob for compiling.
		hr = library->CreateBlobWithEncodingFromPinned( // Create a new blob from the string.
			(LPBYTE)shader.c_str(),						// using the string read in
			(uint32_t)shader.size(),					// of this size
			0,											// no code pages
			&textBlob									// store the result here
		);
		RTX_Exception::handleError(&hr, "Error creating blob from shader text"); // Error handling

		/*CONVERT STD::STRING TO LPCWSTR*/
		std::wstring temp = stringToWstring(_shaderFile);
		LPCWSTR lpcwstrFilename = temp.c_str();


		IDxcOperationResult* result; // Stores the compiled shader
		hr = compiler->Compile(	// Compile a new shader
			textBlob,			// using this blob
			lpcwstrFilename,	// and this file name
			L"",				// from the start
			L"Lib_6_3",			// this profile
			nullptr,			// no arguments
			0,					// no arguments
			nullptr,			// no defines
			0,					// no defines
			includeHandler,		// handler for #includes
			&result			    // store the result here
		);
		RTX_Exception::handleError(&hr, "Error compiling shader"); // Error handling

		// Copy the result in a standard blob to return.
		IDxcBlob* rtnBlob; 
		result->GetResult(&rtnBlob);
		return rtnBlob;
	}

	std::wstring RTX_Pipeline::stringToWstring(std::string _s)
	{
		int len;
		int slength = (int)_s.length() + 1;
		len = MultiByteToWideChar(CP_ACP, 0, _s.c_str(), slength, 0, 0);
		wchar_t* buf = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, _s.c_str(), slength, buf, len);
		std::wstring r(buf);
		delete[] buf;
		return r;
	}

	void RTX_Pipeline::addLibrary(IDxcBlob* _library, const std::vector<std::wstring>& _symbols)
	{
		libraries.emplace_back(Library(_library, _symbols));
	}

	ComPtr<ID3D12RootSignature> RTX_Pipeline::createRayGenSignature()
	{
		std::shared_ptr<RootSignatureGenerator> rootSignatureGen; ///< Generators for root signatures for the shaders.
		rootSignatureGen = std::make_shared<RootSignatureGenerator>();
		rootSignatureGen->addHeapRangesParameter( // Add parameters to heap range
			{
				{
					0, // 0
					1, // descriptor
					0, // implicit register space 0
					D3D12_DESCRIPTOR_RANGE_TYPE_UAV, // type UAV for output buffer
					0 // slot 0 for UAV
				},
				{
					0, // 0
					1, // descriptor
					0, // implicit register space 0
					D3D12_DESCRIPTOR_RANGE_TYPE_SRV, // TLAS
					1 // slot 1 for TLAs
				}
			}
		);
		return rootSignatureGen->generate(rtxManager->getInitializer()->getRTXDevice().Get(), true); // Create a new root signature

	}

	ComPtr<ID3D12RootSignature> RTX_Pipeline::createMissSignature()
	{
		std::shared_ptr<RootSignatureGenerator> rootSignatureGen; ///< Generators for root signatures for the shaders.
		rootSignatureGen = std::make_shared<RootSignatureGenerator>();
		return rootSignatureGen->generate(rtxManager->getInitializer()->getRTXDevice().Get(), true); // Create a new root signature
	}

	ComPtr<ID3D12RootSignature> RTX_Pipeline::createHitSignature()
	{
		std::shared_ptr<RootSignatureGenerator> rootSignatureGen; ///< Generators for root signatures for the shaders.
		rootSignatureGen = std::make_shared<RootSignatureGenerator>();
		rootSignatureGen->addRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV);
		return rootSignatureGen->generate(rtxManager->getInitializer()->getRTXDevice().Get(), true); // Create a new root signature
	}

	int RTX_Pipeline::createShaderLibraries()
	{
		rayGenLibrary = compileShaderLib(rtxManager->getRayGenShader());
		missLibrary = compileShaderLib(rtxManager->getMissShader());
		hitLibrary = compileShaderLib(rtxManager->getHitShader());
		return 0;
	}
	int RTX_Pipeline::addLibraries()
	{
		/*Add the RayGen, Miss, Hit libraries*/
		addLibrary(rayGenLibrary.Get(), { L"RayGen" });
		addLibrary(missLibrary.Get(), { L"Miss" });
		addLibrary(hitLibrary.Get(), { L"Hit" });
		return 0;
	}
	int RTX_Pipeline::createShaderSignatures()
	{
		createRayGenSignature();
		createMissSignature();
		createHitSignature();
		return 0;
	}
	ID3D12StateObject* RTX_Pipeline::generate()
	{
		HRESULT hr; // Error handling

		UINT64 subObjectCount =				  // Count the total amount of sub objects
			libraries.size()			   +  // the amount of libraries
			hitgroups.size()			   +  // the amount of hitgroups
			1							   +  // one shader configuration
			1							   +  // one shader payload
			2 * rootSigAssociations.size() +  // declaration + associations * amount
			2							   +  // the two empty global and local signatures
			1;								  // final pipeline object

		std::vector<D3D12_STATE_SUBOBJECT> subObjects(subObjectCount); // Create a vector to store all of them.

		UINT currentIndex = 0; // Used to help store sub objects.

		for (const Library& lib : libraries) // add all libs
		{
			D3D12_STATE_SUBOBJECT libSubOjbect = {}; // Create a new  subobject
			libSubOjbect.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY; // Set it to lib type
			libSubOjbect.pDesc = &lib.libDesc; // Copy the descriptor

			subObjects[currentIndex] = libSubOjbect; // Add it to the list
			currentIndex++;
		}

		for (const HitGroup& group : hitgroups) // add all hitgroups
		{
			D3D12_STATE_SUBOBJECT hitSubObject = {}; // Create a new subobject
			hitSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP; // Set it to hit group type
			hitSubObject.pDesc = &group.desc; // Copy the descriptor

			subObjects[currentIndex] = hitSubObject; // Add it to the list
			currentIndex++;
		}

		/* Create a shader descriptor with the data passed in earlier.*/
		D3D12_RAYTRACING_SHADER_CONFIG shaderDesc = {};	
		shaderDesc.MaxPayloadSizeInBytes = maxPayLoadSizeInBytes;
		shaderDesc.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;

		/* Create a shader config object to add to the list */
		D3D12_STATE_SUBOBJECT shaderConfigObject = {};
		shaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		shaderConfigObject.pDesc = &shaderDesc;
		subObjects[currentIndex] = shaderConfigObject;
		currentIndex++;

		/* Build a list of symbols */
		std::vector<std::wstring> exportedSymbols = {};
		std::vector<LPCWSTR> exportedSymbolPointers = {};
		buildShaderExportList(exportedSymbols);

		/* Build an array of the pointers */
		exportedSymbolPointers.reserve(exportedSymbols.size()); // Reserve the space
		for (const auto& name : exportedSymbols)
		{
			exportedSymbolPointers.push_back(name.c_str()); // Add the symbols
		}
		const WCHAR** shaderExports = exportedSymbolPointers.data();

		/* Add a sub object for the association of shaders and payload */
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
		shaderPayloadAssociation.NumExports = static_cast<UINT>(exportedSymbols.size());
		shaderPayloadAssociation.pExports = shaderExports;

		/* Associate them */
		shaderPayloadAssociation.pSubobjectToAssociate = &subObjects[(currentIndex - 1)];

		/* Build the payload association object */
		D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
		shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		shaderPayloadAssociationObject.pDesc = &shaderPayloadAssociation;
		subObjects[currentIndex] = shaderPayloadAssociationObject;
		currentIndex++;

		/* Add the root association objects */
		for (RootSignatureAssociation& assoc : rootSigAssociations)
		{
			// First create a subobject for the root sig
			D3D12_STATE_SUBOBJECT rootSigSubobj = {};
			rootSigSubobj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			rootSigSubobj.pDesc = &assoc.rootSignature;
			subObjects[currentIndex] = rootSigSubobj;
			currentIndex++;

			// Copy more data in for the association
			assoc.association.NumExports = static_cast<UINT>(assoc.symbolPointers.size());
			assoc.association.pExports = assoc.symbolPointers.data();
			assoc.association.pSubobjectToAssociate = &subObjects[(currentIndex - 1)];

			// Add a subobject for the association
			D3D12_STATE_SUBOBJECT rootSigAssocSubobj = {};
			rootSigAssocSubobj.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			rootSigAssocSubobj.pDesc = &assoc.association;

			subObjects[currentIndex] = rootSigAssocSubobj;
			currentIndex++;
		}

		/* Add a global and local empty signature */
		D3D12_STATE_SUBOBJECT emptyGlobalRootSig;
		emptyGlobalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		ID3D12RootSignature* dgSig = defaultGlobalSignature;
		emptyGlobalRootSig.pDesc = &dgSig;
		subObjects[currentIndex] = emptyGlobalRootSig;
		currentIndex++;


		D3D12_STATE_SUBOBJECT emptyLocalRootSig;
		emptyLocalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		ID3D12RootSignature* dlSig = defaultLocalSignature;
		emptyLocalRootSig.pDesc = &dlSig;
		subObjects[currentIndex] = emptyLocalRootSig;
		currentIndex++;

		/* Add a subobject for the pipeline config */
		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
		pipelineConfig.MaxTraceRecursionDepth = maxRecursionDepth;
		D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
		pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		pipelineConfigObject.pDesc = &pipelineConfig;
		subObjects[currentIndex] = pipelineConfigObject;
		currentIndex++;

		/* Create a pipeline desc */
		D3D12_STATE_OBJECT_DESC pipelineDesc = {};
		pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		pipelineDesc.NumSubobjects = currentIndex;
		pipelineDesc.pSubobjects = subObjects.data();

		ID3D12StateObject* rtStateObject = nullptr;

		// Create the pipeline
		hr = rtxManager->getInitializer()->getRTXDevice()->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&rtStateObject)); 

		RTX_Exception::handleError(&hr, " Error creating the raytracing pipeline.");

		return rtStateObject;
	}
	int RTX_Pipeline::createShaderResourceHeap()
	{
		srvUavHeap = createDescriptorHeap(				// Create new descriptor heaps
			2,											// 2 of them
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,		// type SRV/UAV/CBV
			true										// visible
		);

		// Get a handle for the srv
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvUavHeap->GetCPUDescriptorHandleForHeapStart();

		// Create a UAV based on the output resource.
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		rtxManager->getInitializer()->getRTXDevice()->CreateUnorderedAccessView( // Create UAV
			rtxManager->getInitializer()->getOutputResource().Get(),			 // using the output resource
			nullptr,															 // no counter
			&uavDesc,															 // use this desc
			srvHandle															 // store here
		);

		// Increment the top level as SRV after the buffer
		srvHandle.ptr += rtxManager->getInitializer()->getRTXDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;										// Create a desc for the SRV 
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;											// Unspecified format
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;  // AS dimension
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;		// default
		srvDesc.RaytracingAccelerationStructure.Location =								// get the TLAS loc
			rtxManager->getBVHManager()->getTLASBuffers().result->GetGPUVirtualAddress();
		
		rtxManager->getInitializer()->getRTXDevice()->CreateShaderResourceView(nullptr, &srvDesc, srvHandle); // Create the SRV

		return 0;
	}
	int RTX_Pipeline::createShaderBindingTable()
	{
		SBTGenerator.reset(); // Reset all.

		// Get the pointer at the beggining of the heap
		D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle = srvUavHeap->GetGPUDescriptorHandleForHeapStart();

		// Reinterpret pointer cause DX12
		auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

		// Add the ray gen program with its data.
		SBTGenerator.addRayGenerationProgram(L"Raygen", { heapPointer });

		// Add the miss and hit program which use no data.
		SBTGenerator.addMissProgram(L"Miss", {});
		SBTGenerator.addRayGenerationProgram(L"HitGroup", { (void*)(rtxManager->getInitializer()->getVertexBuffer()->GetGPUVirtualAddress()) });

		// Calculate the size
		uint32_t sbtsize = SBTGenerator.computeSBTSize();

		sbtStorage = createBuffer(								// Create a new buffer
			rtxManager->getInitializer()->getRTXDevice().Get(), // For this device
			sbtsize,											// This big
			D3D12_RESOURCE_FLAG_NONE,							// No flags
			D3D12_RESOURCE_STATE_GENERIC_READ,					// Read state
			uploadHeapProperties								// upload heap properties
		);

		if (!sbtStorage)
		{
			RTX_Exception::handleError("Error allocating the SBT.", true);
		}
		SBTGenerator.generate(sbtStorage.Get(), rtxManager->getInitializer()->getRTStateObjProperties().Get());
			

		return 0;
	}
	void RTX_Pipeline::setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager)
	{
		rtxManager = _rtxManager;
	}
	void RTX_Pipeline::setMaxPayloadSize(UINT _value)
	{
		maxPayLoadSizeInBytes = _value;
	}
	void RTX_Pipeline::setMaxAttributeSize(UINT _value)
	{
		maxAttributeSizeInBytes = _value;
	}
	void RTX_Pipeline::setMaxRecusionDepth(UINT _value)
	{
		maxRecursionDepth = _value;
	}
	RTX_Pipeline::Library::Library(IDxcBlob* _lib, const std::vector<std::wstring>& _symbols)
		: lib(_lib), symbols(_symbols), exports(_symbols.size())
	{
		// New descriptor for each symbol
		for (size_t i = 0; i < exports.size(); i++)
		{
			exports[i] = {};							// init
			exports[i].Name = symbols[i].c_str();		// set name to symbol
			exports[i].ExportToRename = nullptr;		// no rename
			exports[i].Flags = D3D12_EXPORT_FLAG_NONE;	// no export
		}

		// Creat a lib descriptor
		libDesc.DXILLibrary.BytecodeLength = lib->GetBufferSize();
		libDesc.DXILLibrary.pShaderBytecode = lib->GetBufferPointer();
		libDesc.NumExports = static_cast<UINT>(symbols.size());
		libDesc.pExports = exports.data();
	}
	RTX_Pipeline::Library::Library(const Library& _source)
		: Library(_source.lib, _source.symbols)
	{
	}
	int RTX_Pipeline::RootSignatureGenerator::addHeapRangesParameter(const std::vector<D3D12_DESCRIPTOR_RANGE>& _ranges)
	{
		ranges.push_back(_ranges); // Store the range descriptors.
		D3D12_ROOT_PARAMETER param = {}; // Create new parameters
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;  // Table type
		param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(_ranges.size()); // As many as range decriptors.
		param.DescriptorTable.pDescriptorRanges = nullptr; // Null for now, resolved when generating root signature
		parameters.push_back(param); // Store locally
		rangeLocations.push_back(static_cast<UINT>(ranges.size() - 1)); // Store locally
		return 0;
	}
	int RTX_Pipeline::RootSignatureGenerator::addHeapRangesParameter(std::vector<std::tuple<UINT, UINT, UINT, D3D12_DESCRIPTOR_RANGE_TYPE, UINT>> _ranges)
	{
		std::vector<D3D12_DESCRIPTOR_RANGE> rangeDescs; // Stores descriptors for the ranges.
		for (const auto& input : _ranges) // Loop through all the ranges
		{
			// Create a new descriptor with the params passed in
			D3D12_DESCRIPTOR_RANGE r = {}; 
			r.BaseShaderRegister = std::get<RSC_BASE_SHADER_REGISTER>(input);
			r.NumDescriptors = std::get<RSC_NUM_DESCRIPTORS>(input);
			r.RegisterSpace = std::get<RSC_REGISTER_SPACE>(input);
			r.RangeType = std::get<RSC_RANGE_TYPE>(input);
			r.OffsetInDescriptorsFromTableStart = std::get<RSC_OFFSET_IN_DESCRIPTORS_FROM_TABLE_START>(input);
			rangeDescs.push_back(r);
		}

		// Add the ranges to heap param
		addHeapRangesParameter(rangeDescs);

		return 0;
	}
	int RTX_Pipeline::RootSignatureGenerator::addRootParameter(D3D12_ROOT_PARAMETER_TYPE _type, UINT _shaderRegister, UINT _registerSpace, UINT _numRootConstants)
	{
		D3D12_ROOT_PARAMETER param = {}; // Create a new root parameter
		param.ParameterType = _type; // Pass in the type.

		// Check if its a constant or a buffer and act accordingly.
		if (_type == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
		{
			param.Constants.Num32BitValues = _numRootConstants;
			param.Constants.RegisterSpace = _registerSpace;
			param.Constants.ShaderRegister = _shaderRegister;
		}
		else
		{
			param.Descriptor.RegisterSpace = _registerSpace;
			param.Descriptor.ShaderRegister = _shaderRegister;
		}

		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // Default visibilty is set to global.

		parameters.push_back(param); // Add it to the list
		rangeLocations.push_back(~0u); // Add 0 to specifiy no range

		return 0;
	}
	ID3D12RootSignature* RTX_Pipeline::RootSignatureGenerator::generate(ID3D12Device* _device, bool _local)
	{
		HRESULT hr; // Error handling.
		/*Loop through all params and set the address of the descs bassed on their indices*/
		for (size_t i = 0; i < parameters.size(); i++)
		{
			if (parameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) 
			{
				parameters[i].DescriptorTable.pDescriptorRanges = ranges[rangeLocations[i]].data();
			}
		}

		D3D12_ROOT_SIGNATURE_DESC rootDesc = {}; // Create a new root signature
		rootDesc.NumParameters = static_cast<UINT>(parameters.size()); // Number of parameters specified eariler.
		rootDesc.pParameters = parameters.data(); // Data for params also from earlier
		rootDesc.Flags = _local ?  D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE; // Local signature or not?

		/*Create signature from descriptor*/
		ID3DBlob* sigBlob; // Stores the signature
		ID3DBlob* error; // Stores the error. Note that its not used cause of RTX_Exception but function needs it.

		hr = D3D12SerializeRootSignature(			// Serialize the signature
			&rootDesc,								// using this descriptor
			D3D_ROOT_SIGNATURE_VERSION_1_0,			// version 1
			&sigBlob,								// store the result here
			&error									// and the error here
		);

		RTX_Exception::handleError(&hr, "Failed to serialize root signature");
		ID3D12RootSignature* rootSignature;
		hr = _device->CreateRootSignature( // Create a new root signature
			0,							   // 0 cause single gpu
			sigBlob->GetBufferPointer(),   // use the data from the blob
			sigBlob->GetBufferSize(),	   //
			IID_PPV_ARGS(&rootSignature)   // and store it here
		);
		RTX_Exception::handleError(&hr, "Failed to create root signature");

		return rootSignature;
	}
	RTX_Pipeline::HitGroup::HitGroup(std::wstring _hitGroupName, std::wstring _closestHit, std::wstring _anyHit, std::wstring _intersection)
		: hitGroupName(std::move(_hitGroupName)), closestHitSymbol(std::move(_closestHit)), anyHitSymbol(std::move(_anyHit)), intersectionSymbol(std::move(_intersection))
	{
		desc.HitGroupExport = hitGroupName.c_str();
		desc.ClosestHitShaderImport = closestHitSymbol.empty() ? nullptr : closestHitSymbol.c_str();
		desc.AnyHitShaderImport = anyHitSymbol.empty() ? nullptr : anyHitSymbol.c_str();
		desc.IntersectionShaderImport = intersectionSymbol.empty() ? nullptr : intersectionSymbol.c_str();
	}
	RTX_Pipeline::HitGroup::HitGroup(const HitGroup& _source)
		: HitGroup(_source.hitGroupName, _source.closestHitSymbol, _source.anyHitSymbol, _source.intersectionSymbol)
	{
	}
	RTX_Pipeline::RootSignatureAssociation::RootSignatureAssociation(ID3D12RootSignature* _rootSig, const std::vector<std::wstring>& _symbols)
		: rootSignature(_rootSig), symbols(_symbols), symbolPointers(_symbols.size())
	{
		for (size_t i = 0; i < symbols.size(); i++)
		{
			symbolPointers[i] = symbols[i].c_str();
		}
		rootSignaturePointer = rootSignature;
	}
	RTX_Pipeline::RootSignatureAssociation::RootSignatureAssociation(const RootSignatureAssociation& _source)
		: RootSignatureAssociation(_source.rootSignature, _source.symbols)
	{
	}
}
