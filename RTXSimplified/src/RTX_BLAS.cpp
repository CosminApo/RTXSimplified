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
		bool _isOpaque						// Used to optimize search for closes hit
	)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC descriptor = {};										// Descriptor for input data
		descriptor.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;							// use triangles as base geometry
		descriptor.Triangles.VertexBuffer.StartAddress =									// get the start address from base + offset
			_vertexBuffer->GetGPUVirtualAddress() + _vertexOffsetInBytes;	
		descriptor.Triangles.VertexBuffer.StrideInBytes = _vertexSizeInBytes;				// set the size of each vertex
		descriptor.Triangles.VertexCount = _vertexCount;									// set the number of vertices
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
}