#ifndef RTX_TLAS_H
#define RTX_TLAS_H

#include "d3dx12.h" // DirectX12 api
#include <wrl.h> // Windows Runtime Library -> UINT64
#include <vector> // std::vector
#include "RTX_Exception.h" // Error handling
#include <DirectXMath.h> // XMMATRIX -> 4*4 matrix aligned on a 16-byte boundary 
						 //				that maps to four hardware vector registers

using Microsoft::WRL::ComPtr; ///< Smart pointer for interfaces

// Helper to compute aligned buffer sizes
#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment)        \
  (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

namespace RTXSimplified
{
	struct Instance
	{
		Instance(ID3D12Resource* _blas, const DirectX::XMMATRIX& _tr, UINT _iID, UINT _hgID); ///< Custom constructor.

		ID3D12Resource* bottomLevelAS; ///< BLAS.
		const DirectX::XMMATRIX& transformMat; ///< Transform matrix.
		UINT instanceID; ///< Instance ID visisble in the shader.
		UINT hitGroupIndex; ///< Hit group index to fetch the shaders from the shader binding table.
	}; ///< Structure used to store an instance of the TLAS.
	/**
	*	\brief The class responsible for creating and managing the top level accelleration structure.
	*/
	class RTX_TLAS
	{
	private:
		UINT64 scratchSize; ///< Used to store temporary information.
		UINT64 resultSize; ///< Stores the resulting size of the TLAS instance.
		UINT64 descriptorSize; ///< Stores the resulting size of the TLAS instance.
		std::vector<Instance> instances; ///< Stores the instances contained in the TLAS.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags; ///< Construction flags, indicating whether the AS supports iterative updates.

	public:
		int generate(
			ID3D12GraphicsCommandList4* _commandList, ///< Command list to queue the generation to.
			ID3D12Resource* _scratchBuffer,			 ///< Scratch buffer used. 
			ID3D12Resource* _resultBuffer,			 ///< Stores the AS.
			ID3D12Resource* _descriptorBuffer,		 ///< Stores the descriptors.
			bool _updateOnly,						 ///< True = refit existing AS.
			ID3D12Resource* _previousResult			 ///< Previous AS, used for iterative updates.
		); ///< Generates the TLAS (queues it up on the command list).
		int computeASBufferSize(
			ID3D12Device5* _device,			///< Device on which the build will be performed.
			bool _allowUpdate,				///< if true, allow iterative updates.
			UINT64* _scratchSizeInBytes,	///< Temporary scratch memory.
			UINT64* _resultSizeInBytes,		///< Temporary result memory.
			UINT64* _descriptorsSizeInBytes	///< Temporary descriptor memory.
		); ///< Computes the size of the accelleration structure based on device and data.
		int addInstance(
			ID3D12Resource* _bottomLevelAS,		 ///< BLAS
			const DirectX::XMMATRIX& _transform, ///< Transform matrix.
			UINT _instanceID,					 ///< Instance ID visible in the shader.
			UINT _hitGroupIndex					 ///< Hit group index.
		); ///< Adds instance of TLAS on the GPU.
		
	};
}
#endif // !RTX_TLAS_H
