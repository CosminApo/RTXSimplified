#include "RTX_BLAS.h"

namespace RTXSimplified
{
	int RTX_BLAS::addVertexBuffer(ID3D12Resource* _vertexBuffer, UINT64 _vertexOffsetInBytes, uint32_t _vertexCount,	UINT _vertexSizeInBytes, ID3D12Resource* _transformBuffer, UINT64 _transformOffsetInBytes,	bool _isOpaque)
	{
		addVertexBufferLimited(_vertexBuffer, _vertexOffsetInBytes, _vertexCount, _vertexSizeInBytes, nullptr, 0, 0, _transformBuffer, _transformOffsetInBytes, _isOpaque); // Call the limited variant
		return 0;
	}

	int RTX_BLAS::addVertexBufferLimited(
		ID3D12Resource* _vertexBuffer,		// Contains vertex coords
		UINT64 _vertexOffsetInBytes,		// Offset of the first vertex
		uint32_t _vertexCount,				// Number of vertices in the buffer
		UINT _vertexSizeInBytes,			// Size of a vertex
		ID3D12Resource* _indexBuffer,		// Contains all the vertx indices of triangls
		UINT64 _indexOffsetInBytes,			// Offset to the first index
		uint32_t _indexCount,				// Number of indices in the buffer
		ID3D12Resource* _transformBuffer,	// Contains 4x4 transform matrix
		UINT64 _transformOffsetInBytes,		// Offset of the transform matrix
		bool _isOpaque						// Used to optimize search for c;loses hit
	)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC descriptor = {};										// Descriptor for input data
		descriptor.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;							// use triangles as base geometry
		descriptor.Triangles.VertexBuffer.StartAddress =									// get the start address from base + offset
			_vertexBuffer->GetGPUVirtualAddress() + _vertexOffsetInBytes;	
		descriptor.Triangles.VertexBuffer.StrideInBytes = _vertexSizeInBytes;				// set the size of each vertex
		descriptor.Triangles.VertexCount = _vertexCount;									// set the number of vertices
		descriptor.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		descriptor.Triangles.IndexBuffer =													// get the start of the index buffer, if it exists
			_indexBuffer ? (_indexBuffer->GetGPUVirtualAddress() + _indexOffsetInBytes)
			: 0;
		descriptor.Triangles.IndexFormat = _indexBuffer ? DXGI_FORMAT_R32_UINT				// set the format of the index, if it exists
			: DXGI_FORMAT_UNKNOWN;
		descriptor.Triangles.IndexCount = _indexCount;										// set the amount of indices
		descriptor.Triangles.Transform3x4 =													// set the start of the transform buffer, if it exist
			_transformBuffer ? 
			(_transformBuffer->GetGPUVirtualAddress() + _transformOffsetInBytes)
			: 0;
		descriptor.Flags = _isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE				// set the object to opaque or not
			: D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

		vertexBuffers.push_back(descriptor); // Add the descriptor to the queue

		return 0;
	}

	int RTX_BLAS::computeASBufferSize(
		ID3D12Device5* _device,			    // Device on which the build will be performed
		bool _allowUpdate,					// if true, allow iterative updates
		UINT64* _scratchSizeInBytes,		// Temporary scratch memory
		UINT64* _resultSizeInBytes			// Temporary result memory
	)
	{
		flags = _allowUpdate				// Set whether updates are allowed or not
			? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
			: D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc;					// Contains info about work requested.
		prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;		// its a bottom level AS
		prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;								// how geometry descriptions are specified
		prebuildDesc.NumDescs = static_cast<UINT>(vertexBuffers.size());					// get the number of descs
		prebuildDesc.pGeometryDescs = vertexBuffers.data();									// get the data of the descs
		prebuildDesc.Flags = flags;														// apply flags

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {}; // Represents prebuild information about a raytracing acceleration structure
		_device->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info); // Get the prebuild info
		*_scratchSizeInBytes = ROUND_UP(info.ScratchDataSizeInBytes,	// Set the size of the scratch memory
				D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		*_resultSizeInBytes = ROUND_UP(info.ResultDataMaxSizeInBytes,	//Set the size of the result
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		// Store the data locally
		scratchSize = *_scratchSizeInBytes;
		resultSize = *_resultSizeInBytes;

		return 0;
	}

	int RTX_BLAS::generate(
		ID3D12GraphicsCommandList4* _commandList, // Command list to queue the generation to.
		ID3D12Resource* _scratchBuffer,			 // Scratch buffer used. 
		ID3D12Resource* _resultBuffer,			 // Stores the AS.
		bool _updateOnly,						 // True = refit existing AS.
		ID3D12Resource* _previousResult			 // Previous AS, used for iterative updates.
	)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS localFlags = flags; // Use the flags generated before to check if update or construct.
		if (localFlags == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE && _updateOnly)
			localFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

		/*ERROR CHECKS*/
		// Check you're not trying to update on a non-updateable struct
		if (flags != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE && _updateOnly)
		{
			RTX_Exception::handleError("Trying to update a BLAS not built for updates.", true);
		}
		// Check if the previous BLAS was provided if you're trying to update
		if (_updateOnly && _previousResult == nullptr)
		{
			RTX_Exception::handleError("Trying to update a BLAS but no previous BLAS provided.", true);
		}
		// Check if scratch and result size have been populated
		if (resultSize == 0 || scratchSize == 0)
		{
			RTX_Exception::handleError("Invalid result / scratch size.", true);
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc;						// Descriptor storing info about the BLAS
		buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;	// BLAS type
		buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;							// Array layout
		buildDesc.Inputs.NumDescs = static_cast<UINT>(vertexBuffers.size());				// number of descs based on vertex buffers amount
		buildDesc.Inputs.pGeometryDescs = vertexBuffers.data();								// define the geometry
		buildDesc.Inputs.Flags = localFlags;												// using the flags generated before
		buildDesc.DestAccelerationStructureData = _resultBuffer->GetGPUVirtualAddress();	// get result buffer
		buildDesc.ScratchAccelerationStructureData = _scratchBuffer->GetGPUVirtualAddress();	// get scratch buffer
		buildDesc.SourceAccelerationStructureData = _previousResult ? _previousResult->GetGPUVirtualAddress() : 0; // get previous BLAS if available

		_commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr); // Build the AS

		/*UAV barrier -> used to ensure this buffer is complete before moving on*/
		D3D12_RESOURCE_BARRIER uavBarrier;						// Store info about the UAV barrier
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;		// type Unordered Access View
		uavBarrier.UAV.pResource = _resultBuffer;				// wait for the result buffer
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// no flags
		_commandList->ResourceBarrier(1, &uavBarrier);			// build the barrier

		return 0;
	}
}