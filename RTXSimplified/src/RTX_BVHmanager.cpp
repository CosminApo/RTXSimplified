#include "RTX_BVHmanager.h"
#include "RTX_Manager.h"
#include "RTX_Initializer.h"

namespace RTXSimplified
{
	int RTX_BVHmanager::createBLAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> _vertexBuffers)
	{
		// Add all vertex buffers
		for (const auto& buffer : _vertexBuffers)
		{
			BLASmanager.addVertexBuffer(			// Add a new vertex buffer
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

		BLASmanager.computeASBufferSize(rtxManager->getInitializer()->getRTXDevice().Get(), false, &scratchSizeInBytes, &resultSizeInBytes);

		/*create buffers*/


		return 0;
	}
	int RTX_BVHmanager::createAccelerationStructure()
	{
		
		return 0;
	}
	void RTX_BVHmanager::setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager)
	{
		rtxManager = _rtxManager;
	}
}