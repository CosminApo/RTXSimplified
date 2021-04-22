#ifndef RTX_BLAS_H
#define RTX_BLAS_H

#include <wrl.h> // Windows Runtime Library -> UINT64
#include "d3dx12.h" // DirectX12 api
#include <d3d12.h>
#include <dxcapi.h> // DXR
#include <dxgi1_2.h> // DXR
#include <stdint.h> // uint32_t
#include <vector>
#include "RTX_Exception.h" // Error handling

// Helper to compute aligned buffer sizes
#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment)                                         \
  (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

namespace RTXSimplified
{
	/**
	*	\brief The class responsible for creating and managing the bottom level accelleration structure.
	*/
	class RTX_BLAS
	{
	private:
		UINT64 scratchSize; ///< Used to store temporary information.
		UINT64 resultSize; ///< Stores the resulting size of the BLAS.
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> vertexBuffers = {}; ///< Vertex buffer descriptors used to generate the AS.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags; ///< Flags for the builder.

		int addVertexBufferLimited(
			ID3D12Resource* _vertexBuffer,		///< Contains vertex coords.
			UINT64 _vertexOffsetInBytes,		///< Offset of the first vertex.
			uint32_t _vertexCount,				///< Number of vertices in the buffer.
			UINT _vertexSizeInBytes,			///< Size of a vertex.
			ID3D12Resource* _indexBuffer,		///< Contains all the vertx indices of triangls.
			UINT64 _indexOffsetInBytes,			///< Offset to the first index.
			uint32_t _indexCount,				///< Number of indices in the buffer.
			ID3D12Resource* _transformBuffer,	///< Contains 4x4 transform matrix.
			UINT64 _transformOffsetInBytes,		///< Offset of the transform matrix.
			bool _isOpaque						///< Used to optimize search for closes hit.
		); ///< Allocates the buffers on the GPU. This way, the API limits its use to triangles.
		  
	public:
		int generate(
			ID3D12GraphicsCommandList4* _commandList, ///< Command list to queue the generation to.
			ID3D12Resource* _scratchBuffer,			 ///< Scratch buffer used. 
			ID3D12Resource* _resultBuffer,			 ///< Stores the AS.
			bool _updateOnly,						 ///< True = refit existing AS.
			ID3D12Resource* _previousResult			 ///< Previous AS, used for iterative updates.
		); ///< Generates the BLAS (queues it up on the command list).

		int computeASBufferSize(
			ID3D12Device5* _device,			    ///< Device on which the build will be performed.
			bool _allowUpdate,					///< if true, allow iterative updates.
			UINT64* _scratchSizeInBytes,		///< Temporary scratch memory.
			UINT64* _resultSizeInBytes			///< Temporary result memory.
		); ///< Computes the size of the accelleration structure based on device and data.

		int addVertexBuffer(
			ID3D12Resource* _vertexBuffer,		///< Contains vertex coords.
			UINT64 _vertexOffsetInBytes,		///< Offset of the first vertex.
			uint32_t _vertexCount,				///< Number of vertices in the buffer.
			UINT _vertexSizeInBytes,			///< Size of a vertex.
			ID3D12Resource* _transformBuffer,	///< Contains 4x4 transform matrix.
			UINT64 _transformOffsetInBytes,		///< Offset of the transform matrix.
			bool _isOpaque						///< Used to optimize search for closes hit.
		); ///< Allocates the buffers on the GPU.
	};
}
#endif // !RTX_BLAS_H

