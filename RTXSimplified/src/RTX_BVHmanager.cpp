#include "RTX_BVHmanager.h"
#include "RTX_Manager.h"
#include "RTX_Initializer.h"

namespace RTXSimplified
{
	ID3D12Resource* RTX_BVHmanager::createBuffer(ID3D12Device* _device, uint64_t _size, D3D12_RESOURCE_FLAGS _flags, D3D12_RESOURCE_STATES _initState, const D3D12_HEAP_PROPERTIES& _heapProps)
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

		ID3D12Resource* pBuffer;
		hr = rtxManager->getInitializer()->getRTXDevice().Get()->CreateCommittedResource( //Creates both a resource and an implicit heap and the resource is mapped to the heap
			&_heapProps,					// D3D12_HEAP_PROPERTIES for the heap
			D3D12_HEAP_FLAG_NONE,			// D3D12_HEAP_FLAGS for the heap
			&bufferDesc,					// descriptor for buffer
			_initState,						// D3D12_RESOURCE_STATES 
			nullptr,						// dimension buffer needs nullptr
			IID_PPV_ARGS(&pBuffer)			// store it here
		);
		RTX_Exception::handleError(&hr, "Error creating buffer"); // Handle errors	

		return pBuffer;
	}

	AccelerationStructureBuffers RTX_BVHmanager::createBLAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> _vertexBuffers)
	{
		RTX_BLAS blastemp;
		// Add all vertex buffers
		for (const auto& buffer : _vertexBuffers)
		{
			blastemp.addVertexBuffer(				// Add a new vertex buffer
				buffer.first.Get(),					// from this resource
				0,									// no offset
				buffer.second,						// with these many bufferse
				sizeof(Vertex),						// the stride of one Vertex
				0,									// with no transform buffer
				0,									// nor transform offset
				true								// and make it opaque
			);
		}

		UINT64 scratchSizeInBytes = 0; // Temporary strach space to build AS
		UINT64 resultSizeInBytes = 0;  // Temporary result storage space

		// Calculate the buffer sizes
		blastemp.computeASBufferSize(rtxManager->getInitializer()->getRTXDevice().Get(), false, &scratchSizeInBytes, &resultSizeInBytes);

		AccelerationStructureBuffers buffers; // Struct for buffers info (scratch, result, instanceDesc)
		buffers.scratch = createBuffer(								// Create a buffer
			rtxManager->getInitializer()->getRTXDevice().Get(),		// for this device
			scratchSizeInBytes,										// using the size computed
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,				// allow unordered access
			D3D12_RESOURCE_STATE_COMMON,							// common type
			defaultHeapProperties									// using default heap properties
		);

		buffers.result = createBuffer(								// Create a buffer
			rtxManager->getInitializer()->getRTXDevice().Get(),		// for this device
			resultSizeInBytes,										// using the size computed
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,				// allow unordered access
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,	// acceleration structure type
			defaultHeapProperties									// using default heap properties
		);

		blastemp.generate(											// Generate the new AS
			rtxManager->getInitializer()->getCommandList().Get(),	// using the command list created earlier
			buffers.scratch.Get(),									// and the two buffers we just created
			buffers.result.Get(),									// 
			false,													// this is not and update
			nullptr													// so no previous instance
		);

		return buffers;
	}

	int RTX_BVHmanager::createTLAS(std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& _instances, bool _updateOnly)
	{
		if (!_updateOnly) // If this is generating a TLAS
		{
			if (rtxManager->getShadowsEnabled())
			{
				for (size_t i = 0; i < _instances.size(); i++) // Group up all the instances.
				{
					TLASmanager.addInstance(		// Add a new instance
						_instances[i].first.Get(),	// Using the BLAS
						_instances[i].second,		// and the transform matrix linked to it
						static_cast<UINT>(i),		// with a new ID
						static_cast<UINT>(2 * i));	// and a hit group index of 2 * ID (One for base hit group, one for shadow hit group)
				}
			}
			else
			{
				for (size_t i = 0; i < _instances.size(); i++) // Group up all the instances.
				{
					TLASmanager.addInstance(		// Add a new instance
						_instances[i].first.Get(),	// Using the BLAS
						_instances[i].second,		// and the transform matrix linked to it
						static_cast<UINT>(i),		// with a new ID
						static_cast<UINT>(i));	// and a hit group index of ID (One for base hit group)
				}
			}

			// Note: instance descriptor is also stored on the GPU for this.
			UINT64 scratchSize = 0, resultSize = 0, instanceDescsSize = 0; // Stores the memory needed for each component.
			TLASmanager.computeASBufferSize(							   // Compute the Buffer sizes
				rtxManager->getInitializer()->getRTXDevice().Get(),		   // based on this device
				true,													   // updates allowed
				&scratchSize,
				&resultSize,
				&instanceDescsSize
			);

			TLASBuffers.scratch = createBuffer(							// Create a new buffer
				rtxManager->getInitializer()->getRTXDevice().Get(),		// for this device
				scratchSize,											// using size computed
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,				// allow unordered access
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,					// state required for unordered access
				defaultHeapProperties									// using default heap properties
			);
			TLASBuffers.result = createBuffer(							// Create a new buffer
				rtxManager->getInitializer()->getRTXDevice().Get(),		// for this device
				resultSize,												// using size computed
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,				// allow unordered access
				D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,	// using AS state
				defaultHeapProperties									// using default heap properties
			);
			TLASBuffers.instanceDesc = createBuffer(					// Create a new buffer
				rtxManager->getInitializer()->getRTXDevice().Get(),		// for this device
				instanceDescsSize,										// using size computed
				D3D12_RESOURCE_FLAG_NONE,								// no options specified
				D3D12_RESOURCE_STATE_GENERIC_READ,						// required starting state for an upload heap
				uploadHeapProperties									// using upload heap properties
			);
		}
		TLASmanager.generate(										// Generate the AS
			rtxManager->getInitializer()->getCommandList().Get(),	// using the command list created earlier
			TLASBuffers.scratch.Get(),								// and the three buffers just created
			TLASBuffers.result.Get(),								//
			TLASBuffers.instanceDesc.Get(),							//
			_updateOnly,											// is this an update
			TLASBuffers.result.Get()								// previous instance
		);

		return 0;
	}

	int RTX_BVHmanager::createAccelerationStructure()
	{

		HRESULT hr; // Error handling

		std::vector<AccelerationStructureBuffers> buffers; // Stores all the buffers
		for (int i = 0; i < rtxManager->getModels().size(); i++) // For each model 
		{
			AccelerationStructureBuffers BLASbuffer = createBLAS({ {
				rtxManager->getModels()[i].buffer.Get(),
				rtxManager->getModels()[i].verticesAmount
				} }); // Create a new blas.
			buffers.push_back(BLASbuffer); // And store it locally
		}
		
		// Add all BLAS to instances of the TLAS.
		/*Note -> better way to do this is once per model but for demo purposes its like this*/
		instances =
		{
				{ buffers[0].result, DirectX::XMMatrixIdentity() },
				{ buffers[0].result, DirectX::XMMatrixTranslation(0.f, .5f, 0) },
				{ buffers[0].result, DirectX::XMMatrixTranslation(-.5f, 0.f, 0) },
				{ buffers[1].result, DirectX::XMMatrixTranslation(-.5f, 0.5f, 0) }
		};

		createTLAS(instances); // Create TLAS

		// Flush the command list
		rtxManager->getInitializer()->getCommandList().Get()->Close();
		ID3D12CommandList* commandLists[] = { rtxManager->getInitializer()->getCommandList().Get() };
		rtxManager->getInitializer()->getCommandQueue()->ExecuteCommandLists(1, commandLists);
		rtxManager->getInitializer()->setFenceValue(rtxManager->getInitializer()->getFenceValue() + 1);
		rtxManager->getInitializer()->getCommandQueue()->Signal(
			rtxManager->getInitializer()->getFence().Get(), rtxManager->getInitializer()->getFenceValue());
		rtxManager->getInitializer()->getFence()->SetEventOnCompletion(rtxManager->getInitializer()->getFenceValue(), rtxManager->getInitializer()->getFenceEvent());
		WaitForSingleObject(rtxManager->getInitializer()->getFenceEvent(), INFINITE); // Wait for it to finish

		//Reset the command list
		hr = rtxManager->getInitializer()->getCommandList()->Reset(
			rtxManager->getInitializer()->getCommandAllocator().Get(),
			rtxManager->getInitializer()->getPipelineState().Get()
		);

		RTX_Exception::handleError(&hr, "Error reseting the command list.");

		return 0;
	}
	int RTX_BVHmanager::updateTLAS()
	{
		createTLAS(instances, true);
		return 0;
	}
	AccelerationStructureBuffers RTX_BVHmanager::getTLASBuffers()
	{
		return TLASBuffers;
	}
	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> RTX_BVHmanager::getInstances()
	{
		return instances;
	}
	void RTX_BVHmanager::setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager)
	{
		rtxManager = _rtxManager;
	}
	void RTX_BVHmanager::setInstance(int _instanceNo, int _paramNumber, DirectX::XMMATRIX _valueSecond, ComPtr<ID3D12Resource> _valueFirst)
	{
		if (_paramNumber == 1)
		{
			instances[_instanceNo].first = _valueFirst;
		}
		if (_paramNumber == 2)
		{
			instances[_instanceNo].second = _valueSecond;
		}
	}
}