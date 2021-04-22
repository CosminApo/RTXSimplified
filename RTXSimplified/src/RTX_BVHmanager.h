#ifndef RTX_BVHMANAGER_H
#define RTX_BVHMANAGER_H

#include "RTX_TLAS.h" // TLAS generator
#include "RTX_BLAS.h" // BLAS generator
#include <memory> // smart pointers
#include <DirectXMath.h> // XMFLOAT
#include "RTX_Exception.h"*

namespace RTXSimplified
{
	/*FORWARD DECLARES*/
	class RTX_Manager;

	struct AccelerationStructureBuffers
	{
		ComPtr<ID3D12Resource> scratch;      ///< Scratch memory for AS builder.
		ComPtr<ID3D12Resource> result;       ///< Where the AS is.
		ComPtr<ID3D12Resource> instanceDesc; ///< Hold the matrices of the instances.
	}; ///< Helper struct for storing an AS buffer.

	struct Vertex
	{
		DirectX::XMFLOAT3 position;	///< Position of the vertex.
		DirectX::XMFLOAT4 color;	///< Colour of the vertx.
	}; ///< Stores properties for a vertex.

	static const D3D12_HEAP_PROPERTIES defaultHeapProperties = {
		D3D12_HEAP_TYPE_DEFAULT,			///< Default heap.
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,	///< Unknown cpu page property.
		D3D12_MEMORY_POOL_UNKNOWN,			///< Unkown memory pool.
		0,									///< For multi-adpater operations, 0 = 1 = default
		0									///< For multi-adpater operations, 0 = 1 = default 
	}; ///< Properties for the default heap.

	static const D3D12_HEAP_PROPERTIES uploadHeapProperties = {
		D3D12_HEAP_TYPE_UPLOAD,				///< Upload heap.
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,	///< Unknown cpu page property.
		D3D12_MEMORY_POOL_UNKNOWN,			///< Unkown memory pool.
		0,									///< For multi-adpater operations, 0 = 1 = default
		0									///< For multi-adpater operations, 0 = 1 = default 
	}; ///< Properties for the upload heap.

	/**
	*	\brief The class responsible for creating and managing the bounding volume hierarchy.
	*/
	class RTX_BVHmanager
	{
	private:
		RTX_BLAS BLASmanager; ///< Stores an instance of the bottom level acceleration structure generator.
		RTX_TLAS TLASmanager; ///< Stores an instance of the top level acceleration structure generator.
		AccelerationStructureBuffers buffer; ///< Stores the buffer for the acceleration structure. 
		std::shared_ptr<RTX_Manager> rtxManager; ///< Store a reference to the RTX manager class.
		std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> instances; ///< Stores references to top level acceleration structures.
		AccelerationStructureBuffers TLASBuffers; ///< Storage for the top level acceleration structure buffers.
		ComPtr<ID3D12Resource> bottomLevelAS; ///< Storage for the bottom level acceleration structure.

		AccelerationStructureBuffers createBLAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> _vertexBuffers); ///< Creates the BLAS.
		int createTLAS(std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> _instances); ///< Creates the TLAS.
		ID3D12Resource* createBuffer(ID3D12Device* _device, uint64_t _size, D3D12_RESOURCE_FLAGS _flags,
			D3D12_RESOURCE_STATES _initState, const D3D12_HEAP_PROPERTIES& _heapProps); ///< Creates a buffer based on the device properties, data properties and ctrl flags.

	public:
		int createAccelerationStructure(); ///< Creates the acceleration structure.

		/*SETTERS*/
		void setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager);

	};
}


#endif //!RTX_BVHMANAGER_H