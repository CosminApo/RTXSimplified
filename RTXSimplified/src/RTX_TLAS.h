#ifndef RTX_TLAS_H
#define RTX_TLAS_H

#include "d3dx12.h" // DirectX12 api
#include <wrl.h> // Windows Runtime Library -> UINT64
#include <vector> // std::vector
#include <DirectXMath.h> // XMMATRIX -> 4*4 matrix aligned on a 16-byte boundary 
						 //				that maps to four hardware vector registers

using Microsoft::WRL::ComPtr; ///< Smart pointer for interfaces

namespace RTXSimplified
{
	/**
	*	\brief The class responsible for creating and managing the top level accelleration structure.
	*/
	class RTX_TLAS
	{
	private:
		UINT64 scratchSize; ///< Used to store temporary information.
		UINT64 instanceSize; ///< Stores the resulting size of the TLAS instance.
		std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> instances; ///< Stores references to top level acceleration structures.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags; ///< Construction flags, indicating whether the AS supports iterative updates.
	public:
		int generate(); ///< Generates the BLAS.
		int computeASBufferSize(); ///< Computes the size of the accelleration structure based on device and data.
		int addInstance(); ///< Adds instance of TLAS on the GPU.
	};
}
#endif // !RTX_TLAS_H
