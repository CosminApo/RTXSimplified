#ifndef RTX_BLAS_H
#define RTX_BLAS_H

#include <wrl.h> // Windows Runtime Library -> UINT64

namespace RTXSimplified
{
	/**
	*	\brief The class responsible for creating and managing the bottom level accelleration structure.
	*/
	class RTX_BLAS
	{
	public:
		UINT64 scratchSize; ///< Used to store temporary information.
		UINT64 resultSize; ///< Stores the resulting size of the BLAS.
	private:
		int generate(); ///< Generates the BLAS.
		int computeASBufferSize(); ///< Computes the size of the accelleration structure based on device and data.
		int addVertexBuffer(); ///< Allocates the buffers on the GPU.
	};
}
#endif // !RTX_BLAS_H

