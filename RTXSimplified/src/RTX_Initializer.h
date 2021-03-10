#ifndef RTX_INITIALIZER_H
#define RTX_INITIALIZER_H


namespace RTXSimplified
{
	/**
	*  \brief The class responsible for initializing the library components.
	*
	*	Initializes acceleration structures, raytracing pipeline, shader resource heap
	*	and shader binding table. Also performs checks for RTX support.
	*/
	class RTX_Initializer
	{
	private:
	public:
		int checkRTXSupport(); ///< Checks if the hardware supports RTX.
		int createAccelerationStructures(); ///< Creates acceleration structure. Uses BVHmanager.
		int createRaytracingPipeline(); ///< Creates RT pipeline.
		int createShaderResourceHeap(); ///< Creates shader resource heap.
		int createShaderBindingTable(); ///< Creates shader binding table.
	};
}

#endif // !RTX_INITIALIZER_H