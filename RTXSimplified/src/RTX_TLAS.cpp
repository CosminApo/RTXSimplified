#include "RTX_TLAS.h"

namespace RTXSimplified
{
	Instance::Instance(ID3D12Resource* _blas, const DirectX::XMMATRIX& _tr, UINT _iID, UINT _hgID)
		: bottomLevelAS(_blas), transformMat(_tr), instanceID(_iID), hitGroupIndex(_hgID)
	{
	}
	int RTX_TLAS::generate(ID3D12GraphicsCommandList4* _commandList, ID3D12Resource* _scratchBuffer, ID3D12Resource* _resultBuffer, ID3D12Resource* _descriptorBuffer, bool _updateOnly, ID3D12Resource* _previousResult)
	{
		// Copy the descriptors in the descriptor buffer.
		D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
		_descriptorBuffer->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
		if (!instanceDescs) // Error check
		{
			RTX_Exception::handleError("Failed to map the instance descriptor buffer.", true);
		}

		UINT instanceCount = static_cast<UINT>(instances.size()); // Get the number of instances
		
		if (!_updateOnly) // If this is the first generation
		{
			ZeroMemory(instanceDescs, descriptorSize); // Initialize the memory to be used to 0
		}

		// For each instance
		for (uint32_t i = 0; i < instanceCount; i++)
		{
			/* Make a descriptor */
			instanceDescs[i].InstanceID = instances[i].instanceID; // Copy the iID
			instanceDescs[i].InstanceContributionToHitGroupIndex = instances[i].hitGroupIndex; // Copy the gID
			instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE; // Use default flags
			DirectX::XMMATRIX matrix = DirectX::XMMatrixTranspose(instances[i].transformMat); // Needs to be transposed cause GLM and instance desc mats are different.
			memcpy(instanceDescs[i].Transform, &matrix, sizeof(instanceDescs[i].Transform)); // Copy the matrix
			instanceDescs[i].AccelerationStructure = instances[i].bottomLevelAS->GetGPUVirtualAddress(); // Copy BLAS.
			instanceDescs[i].InstanceMask = 0xFF; // Default always visible value
		}

		_descriptorBuffer->Unmap(0, nullptr);

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS localFlags = flags; // Use the flags generated before to check if update or construct.
		if (localFlags == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE && _updateOnly)
			localFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

		/*ERROR CHECKS*/
		// Check you're not trying to update on a non-updateable struct
		if (flags != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE && _updateOnly)
		{
			RTX_Exception::handleError("Trying to update a TLAS not built for updates.", true);
		}
		// Check if the previous BLAS was provided if you're trying to update
		if (_updateOnly && _previousResult == nullptr)
		{
			RTX_Exception::handleError("Trying to update a TLAS but no previous TLAS provided.", true);
		}
		// Check if scratch, result and descriptor size have been populated
		if (resultSize == 0 || scratchSize == 0 || descriptorSize == 0)
		{
			RTX_Exception::handleError("Invalid result / scratch / descriptor size.", true);
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};						// Descriptor storing info about the TLAS
		buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;	    // TLAS type
		buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;							// Array layout
		buildDesc.Inputs.NumDescs = instanceCount;											// number of descs based on vertex buffers amount
		buildDesc.Inputs.InstanceDescs = _descriptorBuffer->GetGPUVirtualAddress(); 		// define the descriptors
		buildDesc.Inputs.Flags = localFlags;														// using the flags generated before
		buildDesc.DestAccelerationStructureData = _resultBuffer->GetGPUVirtualAddress();	// get result buffer
		buildDesc.ScratchAccelerationStructureData = _scratchBuffer->GetGPUVirtualAddress();// get scratch buffer
		buildDesc.SourceAccelerationStructureData = _updateOnly ? _previousResult->GetGPUVirtualAddress() : 0; // get previous BLAS if available

		_commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr); // Build the AS

		/*UAV barrier -> used to ensure this buffer is complete before moving on*/
		D3D12_RESOURCE_BARRIER uavBarrier;						// Store info about the UAV barrier
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;		// type Unordered Access View
		uavBarrier.UAV.pResource = _resultBuffer;				// wait for the result buffer
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// no flags
		_commandList->ResourceBarrier(1, &uavBarrier);			// build the barrier

		return 0;
	}
	int RTX_TLAS::computeASBufferSize(ID3D12Device5* _device, bool _allowUpdate, UINT64* _scratchSizeInBytes, UINT64* _resultSizeInBytes, UINT64* _descriptorsSizeInBytes)
	{
		flags = _allowUpdate				// Set whether updates are allowed or not
			? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
			: D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = {};			// Contains info about work requested.
		prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;		// its a top level AS
		prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;							// how geometry descriptions are specified
		prebuildDesc.NumDescs = static_cast<UINT>(instances.size());					// the number of descs matches the number of instances
		prebuildDesc.Flags = flags;														// apply flags

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {}; // Stores sizes of scratch and result
		_device->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info); // Get the info from the device
		
		// Buffer sizes alligned to 256-byte-aligned
		info.ResultDataMaxSizeInBytes = ROUND_UP(info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		info.ScratchDataSizeInBytes = ROUND_UP(info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		// Store data locally
		resultSize = info.ResultDataMaxSizeInBytes;
		scratchSize = info.ScratchDataSizeInBytes;
		
		// Calculte descriptor size from instance count
		// size = size of a descriptor * nr of instances, alligned to 256-bytes
		descriptorSize = ROUND_UP(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * static_cast<UINT64>(instances.size()), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		// Feedback results
		*_scratchSizeInBytes = scratchSize;
		*_resultSizeInBytes = resultSize;
		*_descriptorsSizeInBytes = descriptorSize;

		return 0;
	}
	int RTX_TLAS::addInstance(ID3D12Resource* _bottomLevelAS, const DirectX::XMMATRIX& _transform, UINT _instanceID, UINT _hitGroupIndex)
	{
		instances.emplace_back(Instance(_bottomLevelAS, _transform, _instanceID, _hitGroupIndex));
		return 0;
	}
}