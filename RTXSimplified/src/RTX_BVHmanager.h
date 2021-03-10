#ifndef RTX_BVHMANAGER_H
#define RTX_BVHMANAGER_H

#include "RTX_TLAS.h" // TLAS generator
#include "RTX_BLAS.h" // BLAS generator

namespace RTXSimplified
{
	struct AccelerationStructureBuffers
	{
		ComPtr<ID3D12Resource> scratch;      // Scratch memory for AS builder
		ComPtr<ID3D12Resource> result;       // Where the AS is
		ComPtr<ID3D12Resource> instanceDesc; // Hold the matrices of the instances
	};

	/**
	*	\brief The class responsible for creating and managing the bounding volume hierarchy.
	*/
	class RTX_BVHmanager
	{
	private:
		RTX_BLAS BLASmanager; ///< Stores an instance of the bottom level acceleration structure generator.
		RTX_TLAS TLASmanager; ///< Stores an instance of the top level acceleration structure generator.
		AccelerationStructureBuffers buffer; ///< Stores the buffer for the acceleration structure. 
		int createBLAS(); ///< Creates the BLAS.
		int createTLAS(); ///< Creates the TLAS.
	public:
		int createAccelerationStructure(); ///< Creates the acceleration structure.
	};
}


#endif //!RTX_BVHMANAGER_H