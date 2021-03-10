#ifndef RTX_PATHTRACER_H
#define RTX_PATHTRACER_H

#include "d3dx12.h" // DirectX12 api
#include <vector> // std::vectore

namespace RTXSimplified
{
	/**
	*	\brief The class responsible for tracing the path of light rays.
	* 	
	*/
	class RTX_PathTracer
	{
	private:
		D3D12_DISPATCH_RAYS_DESC dispatchDescriptor; ///< Descriptor containing info about the raytracing task.
		std::vector<ID3D12DescriptorHeap*> heaps; ///< Descriptor heap giving access to the top-level acceleration
												  ///< structure, as well as the raytracing output.
	public:
		int traceRays(); ///< Traces the rays of light.
		int createDescriptor(); ///< Creates the descriptor containing info about the RT task.
	};
}

#endif // !RTX_PATHTRACER_H
